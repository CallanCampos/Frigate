#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "worldtrade_local.h"

// The Global Trade Station's lower screen, the trade room: the player, who walks in and out, and the people that
// stand for the trainers a search found. The names are ours, guessed

// The player's walk into or out of the trade room
typedef struct {
    int seq;
    int y;
    WorldTradeWork *wk;
} SubLcdHeroDemo;

static void SubLcd_HeroAnimSet(SubLcdHeroDemo *demo, int seq);
static void SubLcd_HeroDemoTask(TCB *tcb, void *data);
static void SubLcd_ReturnHeroDemoTask(TCB *tcb, void *data);
static void SubLcd_ActPos(ClActor *act, int x, int y);
static int SubLcd_ObjAnimBase(int index);
static void SubLcd_FieldObjLoad(WorldTradeWork *wk);
static void SubLcd_FieldObjTrans(NNSG2dCharacterData **charData, NNSG2dPaletteData *pltt, int index, int trainerType,
                                 int gender);

// Where the characters of each person go in OBJ VRAM
static const u16 sFieldObjCharOffsets[7] = { 0x200, 0x400, 0x600, 0x800, 0xa00, 0xc00, 0xe00 };

// The palette of each trainer type's person
static const u8 sFieldObjPalettes[16] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

// Where the people stand
static const struct {
    u16 x;
    u16 y;
} sSubLcdObjPos[7] = {
    { 128, 50 }, { 96, 51 }, { 160, 51 }, { 64, 66 }, { 192, 66 }, { 48, 98 }, { 208, 98 },
};

static const TouchRect sSubLcdObjTouchRects[] = {
    { 58, 90, 112, 144 },  { 59, 91, 80, 112 },  { 59, 91, 144, 176 },   { 74, 106, 48, 80 },
    { 74, 106, 176, 208 }, { 106, 138, 32, 64 }, { 106, 138, 192, 224 }, { TOUCH_RECT_END, 0, 0, 0 },
};

void WorldTrade_SubLcdActorAdd(WorldTradeWork *wk) {
    ClActorSetup setup;
    ClActorPos pos;
    int i;

    SubLcd_FieldObjLoad(wk);

    setup.x = 128;
    setup.y = 130;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    wk->subAct[0] = ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_CHAR],
                                   wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_PLTT],
                                   wk->clactRes[WT_CLACT_RES_HERO][WT_CLACT_RES_CELL], &setup, 1, HEAPID_WORLDTRADE);
    ClActor_SetAutoAnim(wk->subAct[0], TRUE);
    ClActor_SetBgPriority(wk->subAct[0], 2);
    ClActor_SetSequence(wk->subAct[0], 3);
    ClActor_SetVisible(wk->subAct[0], TRUE);

    for (i = 0; i < 7; i++) {
        wk->subAct[1 + i] =
            ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CHAR],
                           wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_PLTT],
                           wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CELL], &setup, 1, HEAPID_WORLDTRADE);
        ClActor_SetAutoAnim(wk->subAct[1 + i], TRUE);
        ClActor_SetSequence(wk->subAct[1 + i], i * 4 + 14);
        ClActor_SetVisible(wk->subAct[1 + i], FALSE);
        SubLcd_ActPos(wk->subAct[1 + i], sSubLcdObjPos[i].x, sSubLcdObjPos[i].y);
        ClActor_SetBgPriority(wk->subAct[1 + i], 2);
        ClActor_SetPalette(wk->subAct[1 + i], (u8)(i + 2), 1);
    }

    wk->partnerCursorAct =
        ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CHAR],
                       wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_PLTT],
                       wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CELL], &setup, 1, HEAPID_WORLDTRADE);
    ClActor_SetAutoAnim(wk->partnerCursorAct, TRUE);
    ClActor_SetSequence(wk->partnerCursorAct, 43);
    ClActor_SetVisible(wk->partnerCursorAct, FALSE);
    SubLcd_ActPos(wk->partnerCursorAct, 128, 82);
    ClActor_SetBgPriority(wk->partnerCursorAct, 1);

    wk->promptDsAct = ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CHAR],
                                     wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_PLTT],
                                     wk->clactRes[WT_CLACT_RES_SUB][WT_CLACT_RES_CELL], &setup, 1, HEAPID_WORLDTRADE);
    ClActor_SetAutoAnim(wk->promptDsAct, TRUE);
    ClActor_SetSequence(wk->promptDsAct, 42);
    WorldTrade_ActPos(wk->promptDsAct, 55, wk->drawOffset + 168);
    ClActor_SetVisible(wk->promptDsAct, FALSE);

    for (i = 0; i < 8; i++) {
        ClActor_GetSurfacePos(wk->subAct[i], &pos, 1);
        wk->subActY[i][0] = pos.x;
        wk->subActY[i][1] = pos.y;
    }
}

void WorldTrade_HeroDemo(WorldTradeWork *wk) {
    SubLcdHeroDemo *demo;

    WorldTrade_SubLcdActorAdd(wk);
    wk->heroDemoWork = GFL_HeapAllocate(HEAPID_WORLDTRADE, sizeof(SubLcdHeroDemo), FALSE, "worldtrade_sublcd.c", 212);
    wk->demoTask = GFL_TCBMgrAddTask(wk->tcbManager, SubLcd_HeroDemoTask, wk->heroDemoWork, 5);
    demo = GFL_TCBGetData(wk->demoTask);
    demo->seq = 0;
    demo->y = -40;
    demo->wk = wk;
    wk->demoEnd = 0;
    SubLcd_HeroAnimSet(demo, 0);
    GFL_SndSEPlay(SEQ_SE_SYS_86);
}

static void SubLcd_HeroAnimSet(SubLcdHeroDemo *demo, int seq) {
    ClActor_SetSequence(demo->wk->subAct[0], seq);
}

static void SubLcd_HeroDemoTask(TCB *tcb, void *data) {
    SubLcdHeroDemo *demo = data;
    WorldTradeWork *wk = demo->wk;

    switch (demo->seq) {
    case 0:
        if (demo->y > 160) {
            demo->y = 160;
            demo->seq = 1;
            SubLcd_HeroAnimSet(demo, 1);
        }
        demo->y += 2;
        SubLcd_ActPos(wk->subAct[0], 128, demo->y);
        break;
    case 1:
        if (!ClActor_IsAnimActive(wk->subAct[0])) {
            SubLcd_HeroAnimSet(demo, 2);
            demo->seq = 2;
        }
        break;
    case 2:
        if (demo->y <= 138) {
            demo->y = 138;
            demo->seq = 3;
            SubLcd_HeroAnimSet(demo, 3);
        } else {
            demo->y--;
        }
        SubLcd_ActPos(wk->subAct[0], 128, demo->y);
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_PC_LOGIN);
        demo->wk->demoEnd = 1;
        GFL_HeapFree(demo);
        GFL_TCBRemove(tcb);
        break;
    }
}

void WorldTrade_ReturnHeroDemo(WorldTradeWork *wk) {
    SubLcdHeroDemo *demo;

    wk->heroDemoWork = GFL_HeapAllocate(HEAPID_WORLDTRADE, sizeof(SubLcdHeroDemo), FALSE, "worldtrade_sublcd.c", 336);
    wk->demoTask = GFL_TCBMgrAddTask(wk->tcbManager, SubLcd_ReturnHeroDemoTask, wk->heroDemoWork, 5);
    demo = GFL_TCBGetData(wk->demoTask);
    demo->seq = 0;
    demo->y = 138;
    demo->wk = wk;
    SubLcd_HeroAnimSet(demo, 5);
    GFL_SndSEPlay(SEQ_SE_PC_LOGOFF);
}

static void SubLcd_ReturnHeroDemoTask(TCB *tcb, void *data) {
    SubLcdHeroDemo *demo = data;
    WorldTradeWork *wk = demo->wk;

    switch (demo->seq) {
    case 0:
        if (demo->y > 160) {
            demo->y = 160;
            demo->seq = 1;
            SubLcd_HeroAnimSet(demo, 6);
        }
        demo->y += 1;
        SubLcd_ActPos(wk->subAct[0], 128, demo->y);
        break;
    case 1:
        if (!ClActor_IsAnimActive(wk->subAct[0])) {
            SubLcd_HeroAnimSet(demo, 0);
            demo->seq = 2;
            GFL_SndSEPlay(SEQ_SE_SYS_87);
        }
        break;
    case 2:
        if (demo->y < -20) {
            demo->seq = 3;
            SubLcd_HeroAnimSet(demo, 3);
        }
        demo->y -= 2;
        SubLcd_ActPos(wk->subAct[0], 128, demo->y);
        break;
    case 3:
        wk->demoEnd = 1;
        GFL_HeapFree(demo);
        GFL_TCBRemove(tcb);
        break;
    }
}

static void SubLcd_ActPos(ClActor *act, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y - 8;
    ClActor_SetSurfacePos(act, &pos, 1);
}

int WorldTrade_SubLcdObjHitCheck(int count) {
    int ret = GFL_HIDGetTouchPressedRect(sSubLcdObjTouchRects);

    if (ret == TOUCH_RECT_NONE || ret >= count) {
        ret = TOUCH_RECT_NONE;
    }
    return ret;
}

void WorldTrade_SubLcdMatchObjAppear(WorldTradeWork *wk, int count, int appear) {
    int i;

    if (count != 0 && appear == 1) {
        GFL_SndSEPlay(SEQ_SE_FLD_05);
    }
    for (i = 0; i < 7; i++) {
        if (i < count) {
            SubLcd_FieldObjTrans(wk->fieldObjCharaData, wk->fieldObjPalData, i, wk->downloadPokemonData[i].trainerType,
                                 wk->downloadPokemonData[i].gender);
            if (appear) {
                ClActor_SetSequence(wk->subAct[1 + i], i * 4 + 14);
            } else {
                ClActor_SetSequence(wk->subAct[1 + i], i * 4 + 17);
            }
            ClActor_SetVisible(wk->subAct[1 + i], TRUE);
        } else {
            ClActor_SetVisible(wk->subAct[1 + i], FALSE);
        }
    }
}

static int SubLcd_ObjAnimBase(int index) {
    return index * 4 + 14;
}

void WorldTrade_SubLcdMatchObjHide(WorldTradeWork *wk) {
    int i;

    for (i = 0; i < 7; i++) {
        if (wk->subAct[1 + i] != NULL) {
            if (ClActor_GetSequence(wk->subAct[1 + i]) != SubLcd_ObjAnimBase(i) + 1 &&
                ClActor_IsVisible(wk->subAct[1 + i])) {
                ClActor_SetSequence(wk->subAct[1 + i], SubLcd_ObjAnimBase(i) + 1);
            }
        }
    }
}

static void SubLcd_FieldObjLoad(WorldTradeWork *wk) {
    int i;

    wk->fieldObjPalBuf = GFL_G2DIOReadNCLR(ARCID_WIFI_UNIONCHAR, 0, &wk->fieldObjPalData, HEAPID_WORLDTRADE);
    for (i = 0; i < 16; i++) {
        wk->fieldObjCharaBuf[i] =
            GFL_G2DIOReadOBJNCGR(ARCID_WIFI_UNIONCHAR, 0x31 + i, FALSE, &wk->fieldObjCharaData[i], HEAPID_WORLDTRADE);
    }
    for (i = 0; i < 16; i++) {
        DC_FlushRange(wk->fieldObjCharaData[i]->rawData, 0x200);
    }
}

static void SubLcd_FieldObjTrans(NNSG2dCharacterData **charData, NNSG2dPaletteData *pltt, int index, int trainerType,
                                 int gender) {
    u8 *palette;

    if (trainerType > 16) {
        trainerType = 16;
    } else if (trainerType < 0) {
        trainerType = 0;
    }
    palette = pltt->rawData;
    GXS_LoadOBJ(charData[trainerType]->rawData, sFieldObjCharOffsets[index], 0x200);
    GXS_LoadOBJPltt(palette + sFieldObjPalettes[trainerType] * 32, (index + 2) * 32, 32);
}

void WorldTrade_FreeFieldObjData(WorldTradeWork *wk) {
    int i;

    if (wk->demoEnd) {
        if (wk->fieldObjPalBuf != NULL) {
            GFL_HeapFree(wk->fieldObjPalBuf);
            wk->fieldObjPalBuf = NULL;
        }
        for (i = 0; i < 16; i++) {
            if (wk->fieldObjCharaBuf[i] != NULL) {
                GFL_HeapFree(wk->fieldObjCharaBuf[i]);
                wk->fieldObjCharaBuf[i] = NULL;
            }
        }
    }
}

void WorldTrade_SetPartnerCursorPos(WorldTradeWork *wk, int index, int offsetY) {
    SubLcd_ActPos(wk->partnerCursorAct, sSubLcdObjPos[index].x, offsetY + (sSubLcdObjPos[index].y + 32));
}

void WorldTrade_SetPartnerExchangePos(WorldTradeWork *wk) {
    int i;

    for (i = 0; i < 8; i++) {
        if (wk->subAct[i] != NULL) {
            WorldTrade_ActPos(wk->subAct[i], wk->subActY[i][0], wk->subActY[i][1]);
        }
    }
}

void WorldTrade_SetPartnerExchangePosIsReturns(WorldTradeWork *wk) {
    int i;

    for (i = 0; i < 8; i++) {
        if (wk->subAct[i] != NULL) {
            WorldTrade_ActPos(wk->subAct[i], wk->subActY[i][0], wk->subActY[i][1] + 32);
        }
    }
}
