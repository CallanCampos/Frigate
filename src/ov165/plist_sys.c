#include "app/pokelist.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "field/hidden_event.h"
#include "field/musical.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_sync.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/wipe.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/join_avenue.h"
#include "save/trainer_card.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The party list's screen: its graphics, the cursor, and what each mode does with the Pokémon picked. The ROM
// doesn't name this file; it is named for the list's system, which pokelist.c's proc runs

static void PokeList_UpdateGlow(PokeListWork *wk);
static void PokeList_VBlank(TCB *tcb, void *data);
static void PokeList_InitGraphics(PokeListWork *wk);
static void PokeList_ExitGraphics(PokeListWork *wk);
static void PokeList_CreateBG(const BGSetup *setup, u8 bg, u8 mode);
static void PokeList_CreateActors(PokeListWork *wk);
static void PokeList_FreeActors(PokeListWork *wk);
static void PokeList_InitText(PokeListWork *wk);
static void PokeList_ExitText(PokeListWork *wk);
static void PokeList_LoadResources(PokeListWork *wk);
static void PokeList_FreeResources(PokeListWork *wk);
static void PokeList_InitMode(PokeListWork *wk);
static void PokeList_ShowModeMessage(PokeListWork *wk);
static void PokeList_SelectPokemon(PokeListWork *wk);
static void PokeList_OpenMenu(PokeListWork *wk);
static void PokeList_UpdateSelect(PokeListWork *wk);
static void func_ov165_0219bb68(PokeListWork *wk);
static void func_ov165_0219bc7c(PokeListWork *wk);
static void func_ov165_0219be4c(PokeListWork *wk);
static int func_ov165_0219becc(PokeListWork *wk, u8 pos);
static void func_ov165_0219bf40(PokeListWork *wk);
static void func_ov165_0219c038(PokeListWork *wk);
static void func_ov165_0219c20c(PokeListWork *wk);
static void func_ov165_0219c36c(PokeListWork *wk, PartyPkm *from, PartyPkm *to, u16 item);
static void func_ov165_0219c3e8(PokeListWork *wk, PartyPkm *pkmA, PartyPkm *pkmB, u16 itemA, u16 itemB);
static void func_ov165_0219c51c(PartyPkm *pkmA, PartyPkm *pkmB);
static void func_ov165_0219c578(PokeListWork *wk, u32 msgId);
static void func_ov165_0219c5d0(PokeListWork *wk);
static void func_ov165_0219c7e8(PokeListWork *wk);
static void func_ov165_0219c820(PokeListWork *wk);
static void func_ov165_0219c87c(PokeListWork *wk);
static void func_ov165_0219cb80(PokeListWork *wk);
static void func_ov165_0219cdd8(PokeListWork *wk, s32 pos);
static void func_ov165_0219ce34(PokeListWork *wk, s32 pos);
static void PokeList_UpdateMenu(PokeListWork *wk);
static void func_ov165_0219ceec(PokeListWork *wk);
static void func_ov165_0219cf70(PokeListWork *wk);
static void PokeList_DoMenuItem(PokeListWork *wk);
static void PokeList_WaitMessage(PokeListWork *wk);
static void PokeList_UpdateSubMenu(PokeListWork *wk);
static void PokeList_OpenSubMenu(PokeListWork *wk, void (*func)(PokeListWork *wk, u32 item));
static void PokeList_UpdateSwap(PokeListWork *wk);
static void PokeList_StartSwap(PokeListWork *wk);
static void PokeList_EndSwap(PokeListWork *wk);
static void PokeList_AnswerStopLearning(PokeListWork *wk, u32 item);
static void PokeList_AskForgetMove(PokeListWork *wk);
static void PokeList_AnswerForgetMove(PokeListWork *wk, u32 item);
static void PokeList_AnswerSwapItem(PokeListWork *wk, u32 item);
static void PokeList_ShareHpEnd(PokeListWork *wk);
static void PokeList_AnswerTakeMail(PokeListWork *wk, u32 item);
static void PokeList_AskDeleteMail(PokeListWork *wk);
static void PokeList_AnswerDeleteMail(PokeListWork *wk, u32 item);
static void PokeList_SetMove(PokeListWork *wk, PartyPkm *pkm, u8 slot);
static void PokeList_SubFromBag(PokeListWork *wk, u16 item);
static void PokeList_AddToBag(PokeListWork *wk, u16 item);
static BOOL func_ov165_0219da74(PokeListWork *wk);
static BOOL func_ov165_0219da88(PokeListWork *wk);
static void PokeList_LearnMove(PokeListWork *wk, PartyPkm *pkm);
static void PokeList_RaiseFriendship(PokeListWork *wk, PartyPkm *pkm);
static void PokeList_LearnInSlot(PokeListWork *wk);
static void PokeList_AskSwapItem(PokeListWork *wk);
static void PokeList_ShareHpDone(PokeListWork *wk);
static void PokeList_AskTakeMail(PokeListWork *wk);
static void PokeList_MessageDoneDemo(PokeListWork *wk);
static void PokeList_SetHeldItem(PokeListWork *wk, PartyPkm *pkm, u16 item);
static u16 PokeList_GetBagCount(PokeListWork *wk, u16 item);

static const VecFx32 sCameraPos = { 0, 0, FX32_CONST(128) };
static const VecFx32 sCameraUp = { 0, FX32_ONE, 0 };
static const VecFx32 sCameraTarget = { 0, 0, 0 };

static const BGSysLCDConfig sLCDConfig = { 1, 0, 0, 0 };

static const BGSetup sBG5Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xe, 4, 0x8000, 1, 1, 0, 0 };
static const BGSetup sBG6Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xd, 2, 0x8000, 1, 2, 0, 0 };
static const BGSetup sBG7Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xc, 0, 0x6000, 1, 3, 0, 0 };
static const BGSetup sBG0Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xf, 6, 0x8000, 0, 0, 0, 0 };
static const BGSetup sBG1Setup = { 0, 0, 0x1000, 0, 3, GX_BG_COLORMODE_16, 0xa, 2, 0x8000, 0, 1, 0, 0 };
static const BGSetup sBG2Setup = { 0, 0, 0x1000, 0, 3, GX_BG_COLORMODE_16, 0xc, 4, 0x8000, 0, 2, 1, 0 };
static const BGSetup sBG3Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xe, 0, 0x5000, 1, 3, 0, 0 };
static const BGSetup sBG4Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xf, 6, 0x8000, 1, 0, 0, 0 };

BOOL PokeList_Init(PokeListWork *wk) {
    u8 count = PokeParty_GetPkmCount(wk->param->party);
    u8 i;

    wk->touch = func_0203d554();
    wk->menuItem = 0x19;
    wk->state = 0;
    wk->subState = 0;
    wk->cursorPos = 0;
    wk->pkm = NULL;
    wk->selectPos = 9;
    wk->selectPos2 = 9;
    wk->showShortcutButtons = FALSE;
    wk->glowPaused = FALSE;
    wk->enteredCount = 0;
    wk->flashTimer = 0;
    wk->glowAngle = 0;
    wk->unk114 = 0;
    wk->waitTimer = 0;
    wk->hpDoneFunc = NULL;
    wk->statsWindow = NULL;
    wk->pressedButton = NULL;
    wk->buttons[0] = NULL;
    wk->buttons[1] = NULL;
    wk->decided = FALSE;
    wk->demo = 0;
    wk->timeUp = FALSE;
    wk->unk284 = FALSE;
    wk->param->unk73 = 0;
    wk->unk28 = TRUE;
    wk->playHealSe = FALSE;
    for (i = 0; i < POKELIST_CL_RES_COUNT; i++) {
        wk->clResources[i] = CL_RES_NONE;
    }
    wk->wasMode19 = wk->param->mode == 0x19 ? TRUE : FALSE;
    wk->wasMode1B = wk->param->mode == 0x1b ? TRUE : FALSE;
    if (wk->param->mode == 0x18) {
        wk->param->mode = 3;
        wk->wasMode18 = TRUE;
    } else {
        wk->wasMode18 = FALSE;
    }
    if (wk->param->mode == 0x1a) {
        wk->param->mode = 1;
        wk->wasMode1A = TRUE;
    } else {
        wk->wasMode1A = FALSE;
    }
    if (wk->param->mode == 6 && wk->param->move == 0) {
        wk->param->move = PML_ItemGetTMWazaID(wk->param->item);
    }
    PokeList_InitGraphics(wk);
    PokeList_LoadResources(wk);
    PokeList_InitText(wk);
    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        if (i < count) {
            PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, i);

            wk->plates[i] = PokeListPlate_Create(wk, i, pkm);
        } else {
            wk->plates[i] = PokeListPlate_CreateEmpty(wk, i);
        }
        if (PokeListPlate_GetEntry(wk->plates[i]) <= 5) {
            wk->enteredCount++;
        }
    }
    wk->taskMenuRes = AppTaskMenuRes_Create(0, 1, wk->font, wk->printQueue, wk->heapId);
    PokeList_CreateActors(wk);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListBattle_Init(wk);
    }
    wk->vblankTask = GFL_VBlankTCBAdd(PokeList_VBlank, wk, 8);
    wk->message = PokeListMessage_Create(wk);
    wk->menu = PokeListMenu_Create(wk);
    PokeList_InitMode(wk);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysQueueScrLoad(7);
    func_02042ba8(TRUE, wk->heapId);
    return TRUE;
}

BOOL PokeList_Exit(PokeListWork *wk) {
    u8 i;

    GFL_TCBRemove(wk->vblankTask);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListMenu_FreeButton(wk->buttons[0]);
        PokeListMenu_FreeButton(wk->buttons[1]);
        wk->buttons[0] = NULL;
        wk->buttons[1] = NULL;
    }
    PokeListMenu_Free(wk, wk->menu);
    PokeListMessage_Free(wk, wk->message);
    if (wk->statsWindow != NULL) {
        BmpWin_Free(wk->statsWindow);
    }
    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        PokeListPlate_Free(wk, wk->plates[i]);
    }
    AppTaskMenuRes_Free(wk->taskMenuRes);
    PokeList_ExitText(wk);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListBattle_Exit(wk);
    }
    PokeList_FreeActors(wk);
    PokeList_FreeResources(wk);
    PokeList_ExitGraphics(wk);
    if (wk->wasMode18 == TRUE) {
        wk->param->mode = 0x18;
    }
    if (wk->wasMode19 == TRUE) {
        wk->param->mode = 0x19;
    }
    if (wk->wasMode1B == TRUE) {
        wk->param->mode = 0x1b;
    }
    if (wk->wasMode1A == TRUE) {
        wk->param->mode = 0x1a;
    }
    func_0203d564(wk->touch);
    return TRUE;
}

BOOL PokeList_Main(PokeListWork *wk) {
    u8 i;

    switch (wk->state) {
    case 0:
        GFL_WipeSet(WIPE_MODE_SUB_FIRST, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
        wk->state = 1;
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            wk->state = wk->nextState;
        }
        break;
    case 2:
    case 3:
    case 4:
    case 22:
    case 23:
        PokeList_UpdateSelect(wk);
        break;
    case 5:
        PokeList_UpdateSwap(wk);
        break;
    case 6:
        PokeList_UpdateMenu(wk);
        break;
    case 7:
        PokeList_WaitMessage(wk);
        break;
    case 8:
        break;
    case 9:
        PokeList_UpdateSubMenu(wk);
        break;
    case 10:
        PokeListPlate_StartHpChange(wk, wk->plates[wk->cursorPos]);
        if (wk->playHealSe == TRUE) {
            wk->playHealSe = FALSE;
            GFL_SndSEPlay(SEQ_SE_RECOVERY);
        }
        wk->state = 11;
        // fallthrough
    case 11:
        if (PokeListPlate_UpdateHpChange(wk, wk->plates[wk->cursorPos]) == TRUE) {
            if (wk->hpDoneFunc != NULL) {
                wk->hpDoneFunc(wk);
            } else {
                wk->state = 21;
            }
        }
        break;
    case 12:
        wk->waitTimer++;
        if (wk->waitTimer > 16) {
            PokeListMenu_SetButtonPressed(wk->buttons[0], 0);
            if (wk->wasMode1A == TRUE) {
                // Overlay 164's sync takes the parameters as its work
                if (func_ov164_02199944((NetSyncWork *)wk->param, 0, 0) == TRUE) {
                    wk->state = 17;
                    PokeListMenu_FreeButton(wk->buttons[0]);
                    PokeListMenu_FreeButton(wk->buttons[1]);
                    wk->buttons[0] = NULL;
                    wk->buttons[1] = NULL;
                    PokeListMessage_Open(wk, wk->message, 0);
                    PokeListMessage_Print(wk, wk->message, 0xbc);
                    PokeListMessage_ShowWaitIcon(wk, wk->message);
                }
            } else {
                wk->state = 19;
            }
        }
        break;
    case 13:
        PokeList_UpdateLevelUp(wk);
        break;
    case 14:
        PokeListDemo_Start(wk);
        break;
    case 15:
        PokeListDemo_Update(wk);
        break;
    case 16:
        PokeListDemo_End(wk);
        break;
    case 17:
        func_ov164_021999a8((u32)wk->param, 2);
        wk->state = 18;
        break;
    case 18:
        if (func_ov164_021999bc((NetSyncWork *)wk->param, 2) == TRUE) {
            wk->state = 19;
        }
        break;
    case 20:
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        wk->state = 21;
        break;
    case 19:
        if (wk->pressedButton == NULL || func_0204c560(wk->pressedButton) == FALSE) {
            GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
            wk->state = 21;
        }
        break;
    case 21:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }
    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        PokeListPlate_Update(wk, wk->plates[i]);
    }
    PokeListMessage_Update(wk, wk->message);
    PokeList_UpdateGlow(wk);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListBattle_Update(wk);
        PokeListMenu_UpdateButton(wk->buttons[0]);
        PokeListMenu_UpdateButton(wk->buttons[1]);
    }
    func_02021a3c(wk->printQueue);
    func_0204b794();
    return FALSE;
}

// Blends the plates' palettes toward white while a plate flashes, and makes the cursor's plate glow
static void PokeList_UpdateGlow(PokeListWork *wk) {
    u8 i;
    u8 j;

    sys_memcpy16(wk->baseColors, wk->colors, sizeof(wk->colors));
    if (wk->flashTimer != 0) {
        u8 level;

        wk->flashTimer--;
        level = wk->flashTimer;
        if (level > 6) {
            level = 12 - level;
        }
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 16; j++) {
                if (j < 2 || j > 4) {
                    u16 color = wk->baseColors[i][j];
                    u8 g = (color & 0x3e0) >> 5;
                    u8 b = (color & 0x7c00) >> 10;
                    u8 r = color & 0x1f;

                    r = r + level * 2 > 31 ? 31 : r + level * 2;
                    g = g + level * 2 > 31 ? 31 : g + level * 2;
                    b = b + level * 2 > 31 ? 31 : b + level * 2;
                    wk->colors[i][j] = GX_RGB(r, g, b);
                }
            }
        }
    }
    {
        u16 glowColors[3] = { 0x7ff4, 0x7fff, 0x7be9 };
        u16 baseColors[3] = { 0x4a0a, 0x5ef7, 0x4140 };
        fx32 ratio;

        if (wk->glowPaused == FALSE) {
            if (wk->glowAngle + 0x400 >= 0x10000) {
                wk->glowAngle = wk->glowAngle + 0x400 - 0x10000;
            } else {
                wk->glowAngle = wk->glowAngle + 0x400;
            }
        } else {
            wk->glowAngle = 0xc000;
        }
        ratio = (FX_SinIdx(wk->glowAngle) + FX32_ONE) / 2;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                u8 glowG = (glowColors[j] & 0x3e0) >> 5;
                s8 dg = ((u8)((baseColors[j] & 0x3e0) >> 5) - glowG) * ratio >> FX32_SHIFT;
                u8 glowR = glowColors[j] & 0x1f;
                u8 glowB = (glowColors[j] & 0x7c00) >> 10;
                s8 db = ((u8)((baseColors[j] & 0x7c00) >> 10) - glowB) * ratio >> FX32_SHIFT;
                s8 dr = ((u8)(baseColors[j] & 0x1f) - glowR) * ratio >> FX32_SHIFT;
                s32 value;
                u8 r;
                u8 g;
                u8 b;

                value = glowR + dr;
                r = value > 31 ? 31 : value < 0 ? 0 : value;
                value = glowG + dg;
                g = value > 31 ? 31 : value < 0 ? 0 : value;
                value = glowB + db;
                b = value > 31 ? 31 : value < 0 ? 0 : value;
                wk->colors[i][2 + j] = GX_RGB(r, g, b);
            }
        }
    }
    NNS_GfdRegisterNewVramTransferTask(15, 0xc0, wk->colors, sizeof(wk->colors));
}

static void PokeList_VBlank(TCB *tcb, void *data) {
    PokeListWork *wk = data;

    if (wk->glowPaused == TRUE) {
        GX_SetVisibleWnd(GX_WNDMASK_W0);
    } else {
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
    }
    func_0204b7c8();
}

static void PokeList_InitGraphics(PokeListWork *wk) {
    BGSysVRAMConfig vramConfig = {
        GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_NONE,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
    };
    ClActSysSetup clactSetup;

    if (PokeList_IsBattle(wk) == TRUE) {
        vramConfig.objSub = GX_VRAM_SUB_OBJ_128_D;
        vramConfig.texture = GX_VRAM_TEX_NONE;
    } else {
        vramConfig.objSub = GX_VRAM_SUB_OBJ_16_I;
    }
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GFL_BGSysSetVRAMBanks(&vramConfig);
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    PokeList_CreateBG(&sBG3Setup, 3, 0);
    PokeList_CreateBG(&sBG2Setup, 2, 0);
    PokeList_CreateBG(&sBG1Setup, 1, 0);
    PokeList_CreateBG(&sBG7Setup, 7, 0);
    PokeList_CreateBG(&sBG6Setup, 6, 0);
    PokeList_CreateBG(&sBG5Setup, 5, 0);
    PokeList_CreateBG(&sBG4Setup, 4, 0);
    PokeList_CreateBG0(wk);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, GX_PLANEMASK_BG3, 16, 10);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 16, 10);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, 128);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, 128);
    clactSetup = data_02093f08;
    ClActSys_Create(&clactSetup, &vramConfig, wk->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GfdClearVramTransferQueue();
}

void PokeList_CreateBG0(PokeListWork *wk) {
    gfxSetEngineModeA(1, 0, 0);
    PokeList_CreateBG(&sBG0Setup, 0, 0);
}

void PokeList_Init3D(PokeListWork *wk) {
    gfxSetEngineModeA(1, 0, 1);
    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, wk->heapId, NULL);
    G2_SetBG0Priority(0);
    G3X_EdgeMarking(TRUE);
    G3X_AntiAlias(TRUE);
    G3X_AlphaBlend(TRUE);
    wk->camera = GFL_G3DCameraCreate(2, 0, 0x50d7, 0, 0x6bca, FX32_ONE, FX32_CONST(128), 0, &sCameraPos, &sCameraUp,
                                     &sCameraTarget, wk->heapId);
    GFL_G3DCameraFlush(wk->camera);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2,
                        GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 16, 10);
    func_0204f918(wk->heapId);
}

static void PokeList_ExitGraphics(PokeListWork *wk) {
    GfdClearVramTransferQueue();
    func_0204b758();
    PokeList_ReleaseBG0(wk);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

void PokeList_ReleaseBG0(PokeListWork *wk) {
    GFL_BGSysReleaseBG(0);
}

void PokeList_Exit3D(PokeListWork *wk) {
    func_0204fb4c();
    GFL_G3DCameraFree(wk->camera);
    GFL_G3DSysFree();
    GFL_BGSysReleaseBG(0);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, GX_PLANEMASK_BG3, 16, 10);
}

static void PokeList_CreateBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void PokeList_CreateActors(PokeListWork *wk) {
    ClActorSetup cursorSetup;
    ClActorSetup setup;

    wk->actorUnit = func_0204bf1c(10, 8, wk->heapId);
    func_0204c028(wk->actorUnit);
    cursorSetup.x = 0;
    cursorSetup.y = 0;
    cursorSetup.sequence = 0;
    cursorSetup.priority = 10;
    cursorSetup.bgPriority = 3;
    wk->cursor = func_0204c040(wk->actorUnit, wk->clResources[CL_RES_CHAR(0)], wk->clResources[CL_RES_PLTT(0)],
                               wk->clResources[CL_RES_CELL(0)], &cursorSetup, 0, wk->heapId);
    wk->subCursor = func_0204c040(wk->actorUnit, wk->clResources[CL_RES_CHAR(0)], wk->clResources[CL_RES_PLTT(0)],
                                  wk->clResources[CL_RES_CELL(0)], &cursorSetup, 0, wk->heapId);
    func_0204c520(wk->cursor, TRUE);
    func_0204c520(wk->subCursor, TRUE);
    func_0204c124(wk->cursor, FALSE);
    func_0204c124(wk->subCursor, FALSE);
    setup.x = 224;
    setup.y = 168;
    setup.sequence = 1;
    setup.priority = 0;
    setup.bgPriority = 0;
    wk->exitButton = func_0204c040(wk->actorUnit, wk->clResources[CL_RES_CHAR(2)], wk->clResources[CL_RES_PLTT(1)],
                                   wk->clResources[CL_RES_CELL(2)], &setup, 0, wk->heapId);
    func_0204c520(wk->exitButton, TRUE);
    setup.x = 204;
    setup.sequence = 0;
    wk->shortcutButton = func_0204c040(wk->actorUnit, wk->clResources[CL_RES_CHAR(2)], wk->clResources[CL_RES_PLTT(1)],
                                       wk->clResources[CL_RES_CELL(2)], &setup, 0, wk->heapId);
    func_0204c520(wk->shortcutButton, TRUE);
    setup.y = 172;
    setup.x = 184;
    setup.sequence = 6;
    wk->shortcutMark = func_0204c040(wk->actorUnit, wk->clResources[CL_RES_CHAR(2)], wk->clResources[CL_RES_PLTT(1)],
                                     wk->clResources[CL_RES_CELL(2)], &setup, 0, wk->heapId);
    func_0204c520(wk->shortcutMark, TRUE);
}

static void PokeList_FreeActors(PokeListWork *wk) {
    func_0204c108(wk->exitButton);
    func_0204c108(wk->shortcutButton);
    func_0204c108(wk->shortcutMark);
    func_0204c108(wk->cursor);
    func_0204c108(wk->subCursor);
    func_0204bf98(wk->actorUnit);
}

static void PokeList_InitText(PokeListWork *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_11, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1c0, 0x20, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    wk->printQueue = func_020219a8(0x1800, wk->heapId);
    func_020232d8();
}

static void PokeList_ExitText(PokeListWork *wk) {
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->smallFont);
    GFL_FontFree(wk->font);
}

static void PokeList_LoadResources(PokeListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0x4b, wk->heapId);
    ArcTool *iconArc;
    ArcTool *commonArc;

    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0, 0, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0x1a0, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0xf, 3, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x14, 3, 0, 0, FALSE, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0xc, 2, 0, 0, FALSE, wk->heapId);
    wk->plateScreenFile = GFL_G2DIOReadNSCRArc(arc, 0x12, FALSE, &wk->plateScreen, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, 4, 0, 0, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0xe, 7, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x13, 7, 0, 0, FALSE, wk->heapId);
    wk->clResources[CL_RES_PLTT(0)] = func_0204bbb8(arc, 4, 0, 0, 0, 3, wk->heapId);
    wk->clResources[CL_RES_CHAR(0)] = func_0204b81c(arc, 0xb, FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CHAR(1)] = func_0204b81c(arc, 8, FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(0)] = func_0204bde0(arc, 0x17, 0x1a, wk->heapId);
    wk->clResources[CL_RES_CELL(1)] = func_0204bde0(arc, 0x16, 0x19, wk->heapId);
    GFL_ArcToolFree(arc);

    iconArc = GFL_ArcSysCreateFileHandle(7, wk->heapId);
    wk->clResources[CL_RES_PLTT(6)] = func_0204bc48(iconArc, func_02021114(), 0, 0xc0, wk->heapId);
    wk->clResources[CL_RES_CELL(7)] = func_0204bde0(iconArc, func_0202111c(), getOBJTileMapping_MainEng(), wk->heapId);
    GFL_ArcToolFree(iconArc);

    commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    // BUG: The BG of the common bar is loaded from the closed handle of the first archive, rather than the common
    // archive's, which works as long as the handle's memory is given to the new handle
#ifdef BUGFIX
    GFL_G2DIOLoadArcNCLRDefault(commonArc, func_0202d820(), 0, 0x160, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(commonArc, func_0202d824(), 1, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(commonArc, func_0202d82c(), 1, 0, 0, FALSE, wk->heapId);
#else
    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), 0, 0x160, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), 1, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d82c(), 1, 0, 0, FALSE, wk->heapId);
#endif
    GFL_BGSysSetScrPaletteNo(1, 0, 0, 0x40, 0x20, 0xb);
    wk->clResources[CL_RES_PLTT(5)] = func_0204bbb8(commonArc, func_0202d954(), 0, 0xa0, 0, 1, wk->heapId);
    wk->clResources[CL_RES_CHAR(6)] = func_0204b81c(commonArc, func_0202d958(2), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(6)] = func_0204bde0(commonArc, func_0202d95c(2), func_0202d960(2), wk->heapId);
    wk->clResources[CL_RES_PLTT(1)] = func_0204bbb8(commonArc, func_0202d810(), 0, 0x120, 0, 3, wk->heapId);
    wk->clResources[CL_RES_CHAR(2)] = func_0204b81c(commonArc, func_0202d814(), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(2)] = func_0204bde0(commonArc, func_0202d818(2), func_0202d81c(2), wk->heapId);
    wk->clResources[CL_RES_PLTT(2)] = func_0204bbb8(commonArc, func_0202d890(), 0, 0x60, 0, 1, wk->heapId);
    wk->clResources[CL_RES_CHAR(3)] = func_0204b81c(commonArc, func_0202d894(), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(3)] = func_0204bde0(commonArc, func_0202d898(2), func_0202d89c(2), wk->heapId);
    wk->clResources[CL_RES_PLTT(4)] = func_0204bbb8(commonArc, func_0202d8b0(), 0, 0x80, 0, 1, wk->heapId);
    wk->clResources[CL_RES_CHAR(5)] = func_0204b81c(commonArc, func_0202d8b4(), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(5)] = func_0204bde0(commonArc, func_0202d8b8(2), func_0202d8bc(2), wk->heapId);
    GFL_ArcToolFree(commonArc);
    sys_memcpy16((void *)(HW_BG_PLTT + 0xc0), wk->baseColors, sizeof(wk->baseColors));
}

static void PokeList_FreeResources(PokeListWork *wk) {
    u8 i;

    GFL_HeapFree(wk->plateScreenFile);
    for (i = 0; i < CL_RES_PLTT_COUNT; i++) {
        if (wk->clResources[i] != CL_RES_NONE) {
            func_0204bcd0(wk->clResources[i]);
        }
    }
    for (i = CL_RES_CHAR(0); i < CL_RES_CHAR(CL_RES_CHAR_COUNT); i++) {
        if (wk->clResources[i] != CL_RES_NONE) {
            func_0204b98c(wk->clResources[i]);
        }
    }
    for (i = CL_RES_CELL(0); i < CL_RES_CELL(CL_RES_CELL_COUNT); i++) {
        if (wk->clResources[i] != CL_RES_NONE) {
            func_0204be64(wk->clResources[i]);
        }
    }
}

static void PokeList_InitMode(PokeListWork *wk) {
    switch (wk->param->mode) {
    case 1:
        wk->buttons[0] = PokeListMenu_CreateButton(wk, wk->menu, 0, 22, 21, 0);
        PokeList_ShowModeMessage(wk);
        wk->nextState = 2;
        break;
    case 22:
    case 23:
        wk->buttons[0] = PokeListMenu_CreateButton(wk, wk->menu, 0, 12, 21, 0);
        wk->buttons[1] = PokeListMenu_CreateButton(wk, wk->menu, 1, 22, 21, 1);
        PokeList_ShowModeMessage(wk);
        wk->nextState = 2;
        break;
    case 0:
    case 6:
    case 9:
    case 14:
    case 16:
    case 18:
    case 21:
    case 25:
    case 27:
        PokeList_ShowModeMessage(wk);
        wk->nextState = 2;
        break;
    case 3:
        wk->showShortcutButtons = FALSE;
        PokeList_ShowModeMessage(wk);
        wk->nextState = 2;
        break;
    case 5:
        if (PokeList_IsItemForParty(wk, wk->param->item) == TRUE) {
            s32 pos = PokeList_FindItemTarget(wk);

            if (pos != -1) {
                wk->cursorPos = pos;
                wk->pkm = PokeParty_GetPkm(wk->param->party, pos);
                PokeList_ShowItemResult(wk, 0);
                StatusRcv_UseItem(wk->pkm, wk->param->item, 0, wk->param->zoneId, wk->heapId);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                wk->playHealSe = TRUE;
                wk->nextState = wk->state;
                wk->state = 0;
                PokeList_SubFromBag(wk, wk->param->item);
            } else {
                PokeList_ShowItemUselessExit(wk);
                wk->nextState = wk->state;
                wk->state = 0;
            }
        } else if (PokeList_GetBagCount(wk, wk->param->item)) {
            PokeList_ShowModeMessage(wk);
            wk->nextState = 2;
        } else {
            wk->param->result = 10;
            PokeListMessage_Close(wk, wk->message);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetItemName(wk, wk->message, 0, wk->param->item);
            PokeList_ShowMessage(wk, 0xbf, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            wk->nextState = wk->state;
            wk->state = 0;
        }
        break;
    case 7:
    case 8:
        wk->cursorPos = wk->param->index;
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->param->index);
        if (wk->param->moveSlot < 4) {
            u32 move = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + wk->param->moveSlot, NULL);

            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, move);
            PokeList_ShowMessage(wk, 0x29, TRUE, PokeList_LearnInSlot);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, 0x24, FALSE, PokeList_AskForgetMove);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        wk->nextState = 7;
        wk->state = 0;
        wk->showShortcutButtons = FALSE;
        PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
        break;
    case 10:
        wk->cursorPos = wk->param->index;
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->param->index);
        if (wk->param->item == 0) {
            wk->param->mode = 0;
            PokeList_ShowModeMessage(wk);
            wk->nextState = 2;
        } else {
            u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

            if (heldItem == 0) {
                PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                PokeListMessage_CreateWordSet(wk, wk->message);
                PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
                PokeListMessage_SetItemName(wk, wk->message, 1, wk->param->item);
                PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
                PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
                if (PokeListDemo_CanBecomeOrigin(wk, wk->pkm) == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, 0x5c, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = 1;
                } else {
                    PokeList_ShowMessage(wk, 0x5c, TRUE, PokeList_MessageDoneSelect);
                }
                PokeListMessage_FreeWordSet(wk, wk->message);
                wk->param->mode = 0;
            } else {
                PokeListMessage_CreateWordSet(wk, wk->message);
                PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
                PokeListMessage_SetItemTextName(wk, wk->message, 1, heldItem);
                PokeList_ShowMessage(wk, 0x3b, FALSE, PokeList_AskSwapItem);
                PokeListMessage_FreeWordSet(wk, wk->message);
            }
        }
        wk->nextState = 7;
        wk->state = 0;
        wk->showShortcutButtons = TRUE;
        PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
        break;
    case 11:
    case 12: {
        u32 heldItem;
        u32 msg;

        wk->cursorPos = wk->param->index;
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->param->index);
        heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);
        PokeListMessage_CreateWordSet(wk, wk->message);
        if (heldItem == 0) {
            msg = 0x5c;
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, wk->param->item);
        } else {
            msg = 0x41;
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, heldItem);
            PokeListMessage_SetItemName(wk, wk->message, 2, wk->param->item);
        }
        PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
        PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
        PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
        PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
        if (PokeListDemo_CanBecomeAltered(wk, wk->pkm) == TRUE) {
            PokeListDemo_SetAltered(wk, wk->pkm);
            PokeList_ShowMessage(wk, msg, TRUE, PokeList_MessageDoneDemo);
            wk->demo = 2;
        } else if (wk->param->mode == 11) {
            PokeList_ShowMessage(wk, msg, TRUE, PokeList_MessageDoneExit);
        } else {
            PokeList_ShowMessage(wk, msg, TRUE, PokeList_MessageDoneSelect);
        }
        PokeListMessage_FreeWordSet(wk, wk->message);
        if (wk->param->mode == 11) {
            wk->param->result = 10;
        } else {
            wk->param->mode = 0;
        }
        wk->nextState = 7;
        wk->state = 0;
        wk->showShortcutButtons = TRUE;
        break;
    }
    }
}

static void PokeList_ShowModeMessage(PokeListWork *wk) {
    GFL_BGSysLoadScr(0);
    switch (wk->param->mode) {
    case 0:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 9);
        wk->showShortcutButtons = TRUE;
        break;
    case 1:
        PokeListBattle_ShowMessage(wk);
        func_0204c124(wk->exitButton, FALSE);
        wk->showShortcutButtons = FALSE;
        break;
    case 22:
    case 23:
        wk->showShortcutButtons = FALSE;
        break;
    case 5:
    case 16:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 0xd);
        wk->showShortcutButtons = FALSE;
        wk->unk28 = FALSE;
        break;
    case 9:
    case 14:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 0xc);
        wk->showShortcutButtons = FALSE;
        wk->unk28 = FALSE;
        break;
    case 6:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 0xe);
        wk->showShortcutButtons = FALSE;
        wk->unk28 = FALSE;
        break;
    case 18:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 0x10);
        wk->showShortcutButtons = FALSE;
        break;
    case 3:
    case 25:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 9);
        wk->showShortcutButtons = FALSE;
        break;
    case 21:
    case 27:
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 0xaf);
        func_0204c124(wk->exitButton, FALSE);
        wk->showShortcutButtons = FALSE;
        break;
    }
    func_ov165_0219bb68(wk);
}

// A Pokémon was picked: what happens depends on the mode
static void PokeList_SelectPokemon(PokeListWork *wk) {
    if (wk->param->unk73 == 1) {
        wk->state = 19;
        return;
    }
    switch (wk->param->mode) {
    case 0:
    case 1:
    case 18:
    case 21:
    case 22:
    case 23:
    case 25:
        func_ov165_0219ceec(wk);
        GFL_BGSysLoadScr(0);
        PokeList_OpenMenu(wk);
        break;
    case 5:
        if (PokeList_IsItemForMove(wk, wk->param->item) == TRUE) {
            func_ov165_0219ceec(wk);
            PokeList_OpenMenu(wk);
        } else if (StatusRcv_CanUseItem(wk->pkm, wk->param->item, 0, wk->heapId) == TRUE) {
            u32 result = PokeList_ShowItemResult(wk, 0);
            BOOL used = StatusRcv_UseItem(wk->pkm, wk->param->item, 0, wk->param->zoneId, wk->heapId);

            if (result == 2) {
                if (used) {
                    func_02038bc8(9);
                }
                PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], wk->pkm, 0);
            } else {
                if (result == 19 || result == 13) {
                    PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], wk->pkm, 0);
                } else {
                    PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                }
                GFL_SndSEPlay(SEQ_SE_RECOVERY);
            }
            PokeList_SubFromBag(wk, wk->param->item);
        } else if (wk->param->item == ITEM_GRACIDEA && PokeListDemo_CanBecomeSky(wk, wk->pkm) == TRUE) {
            PokeListDemo_SetSky(wk, wk->pkm);
            wk->param->result = 10;
            wk->state = 14;
            wk->demo = 3;
        } else if (wk->param->item == ITEM_REVEAL_GLASS && PokeListDemo_CanBecomeTherian(wk, wk->pkm) == TRUE &&
                   isOneShotDRObtained(wk->param->trainerCard, 6, wk->param->playerInfo) == TRUE) {
            PokeListDemo_ToggleTherian(wk, wk->pkm);
            wk->param->result = 10;
            wk->state = 14;
            wk->demo = 6;
        } else if (wk->param->item == ITEM_DNA_SPLICERS_FUSE) {
            u32 result = PokeListDemo_CheckFuse(wk, wk->pkm);

            if (result == 2 || result == 4) {
                PokeList_ShowItemUseless(wk);
            } else if (result == 3) {
                PokeList_ShowItemMessageSelect(wk, 1);
            } else if (result == 0) {
                func_ov165_0219cdd8(wk, wk->cursorPos);
                func_ov165_0219ce34(wk, wk->cursorPos);
                wk->selectPos = wk->cursorPos;
                wk->param->result = 10;
                wk->state = 23;
                wk->showShortcutButtons = FALSE;
                PokeListMessage_Open(wk, wk->message, 0);
                PokeListMessage_Print(wk, wk->message, 0x12);
                func_ov165_0219bb68(wk);
            }
        } else if (wk->param->item == ITEM_DNA_SPLICERS_SEPARATE) {
            u32 result = PokeListDemo_CheckSeparate(wk, wk->pkm);

            if (result == 5) {
                PokeList_ShowItemMessageSelect(wk, 2);
            } else if (result == 2 || result == 4) {
                PokeList_ShowItemUseless(wk);
            } else {
                wk->selectPos = wk->cursorPos;
                wk->param->result = 10;
                wk->state = 14;
                wk->demo = 5;
                wk->demoForm = 2;
            }
        } else {
            PokeList_ShowItemUseless(wk);
        }
        break;
    case 16:
        if (PokeList_CanEvolveWithItem(wk, wk->pkm, wk->param->item)) {
            PokeList_SubFromBag(wk, wk->param->item);
            wk->state = 19;
            wk->param->index = wk->cursorPos;
            wk->param->result = 8;
        } else {
            PokeList_ShowMessage(wk, 0x52, TRUE, PokeList_MessageDoneExit);
        }
        break;
    case 9: {
        u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        if (heldItem == 0) {
            if (PML_ItemIsMail(wk->param->item) == TRUE) {
                wk->state = 19;
                wk->param->index = wk->cursorPos;
                wk->param->result = 6;
            } else {
                wk->param->result = 10;
                PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                PokeListMessage_CreateWordSet(wk, wk->message);
                PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
                PokeListMessage_SetItemName(wk, wk->message, 1, wk->param->item);
                PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
                PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
                if (PokeListDemo_CanBecomeOrigin(wk, wk->pkm) == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, 0x5c, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = 1;
                } else {
                    PokeList_ShowMessage(wk, 0x5c, TRUE, PokeList_MessageDoneExit);
                }
                PokeListMessage_FreeWordSet(wk, wk->message);
            }
        } else if (PML_ItemIsMail(heldItem) == TRUE) {
            wk->param->result = 10;
            PokeList_ShowMessage(wk, 0x3a, TRUE, PokeList_MessageDoneExit);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemTextName(wk, wk->message, 1, heldItem);
            PokeList_ShowMessage(wk, 0x3b, FALSE, PokeList_AskSwapItem);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    }
    case 14:
        if (PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL) == 0) {
            MailData *mail = func_020097f4(wk->param->unk08, 0, wk->param->unk6C, wk->heapId);

            PokeParty_SetParam(wk->pkm, PKM_PARAM_MAIL, (u32)mail);
            GFL_HeapFree(mail);
            PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, wk->param->item);
            func_020097d0(wk->param->unk08, 0, wk->param->unk6C);
            PokeList_ShowMessage(wk, 0x6c, TRUE, PokeList_MessageDoneExit);
        } else {
            PokeList_ShowMessage(wk, 0x6d, TRUE, PokeList_MessageDoneExit);
        }
        break;
    case 6:
        switch (PokeList_CheckLearnMove(wk, wk->pkm, wk->param->index)) {
        case 0:
            wk->param->result = 10;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, 0x2a, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            PokeList_LearnMove(wk, wk->pkm);
            PokeList_RaiseFriendship(wk, wk->pkm);
            break;
        case 1:
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, 0x21, FALSE, PokeList_AskStopLearning);
            PokeListMessage_FreeWordSet(wk, wk->message);
            break;
        case 2:
            wk->param->result = 10;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, 0x2b, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            break;
        case 3:
            wk->param->result = 10;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, 0x2c, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            break;
        }
        break;
    case 3:
    case 27:
        wk->state = 19;
        wk->param->index = wk->cursorPos;
        wk->param->result = 0;
        break;
    default:
        func_ov165_0219ceec(wk);
        PokeList_OpenMenu(wk);
        break;
    }
}

// Opens the menu of what to do with the Pokémon picked
static void PokeList_OpenMenu(PokeListWork *wk) {
    u32 items[8];

    GFL_BGSysLoadScr(0);
    switch (wk->param->mode) {
    case 0:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = 0;
            items[1] = 3;
            items[2] = 6;
            items[3] = 16;
        } else {
            items[0] = 0;
            items[1] = 1;
            items[2] = 3;
            items[3] = 4;
            items[4] = 6;
            items[5] = 16;
            if (PML_ItemIsMail(PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL)) == TRUE) {
                items[3] = 5;
            }
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, 0x13);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case 1:
    case 22:
    case 23:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = 0;
            items[1] = 6;
            items[2] = 16;
        } else {
            items[0] = 8;
            items[1] = 0;
            items[2] = 6;
            items[3] = 16;
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, 0x13);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case 5:
        items[0] = 2;
        items[1] = 6;
        items[2] = 16;
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, PokeList_GetItemMenuMessage(wk, wk->param->item));
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case 18:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = 0;
            items[1] = 6;
            items[2] = 16;
        } else {
            items[0] = 7;
            items[1] = 0;
            items[2] = 6;
            items[3] = 16;
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, 0x13);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case 21:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = 11;
            items[1] = 0;
            items[2] = 6;
            items[3] = 16;
            PokeListMessage_Open(wk, wk->message, 2);
            PokeListMessage_Print(wk, wk->message, 0xae);
        } else {
            items[0] = 0;
            items[1] = 6;
            items[2] = 16;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_Open(wk, wk->message, 2);
            PokeListMessage_Print(wk, wk->message, 0x13);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    case 25:
        if (MusicalSystem_CanJoin(wk->pkm) == TRUE) {
            items[0] = 11;
            items[1] = 0;
            items[2] = 6;
            items[3] = 16;
        } else {
            items[0] = 0;
            items[1] = 6;
            items[2] = 16;
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, 0x13);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    default:
        items[0] = 0;
        items[1] = 6;
        items[2] = 16;
        break;
    }
    PokeListMenu_Open(wk, wk->menu, items);
}

static void PokeList_UpdateSelect(PokeListWork *wk) {
    switch (wk->subState) {
    case 0:
        wk->subState = 1;
        wk->input = 4;
        break;
    case 1:
        func_ov165_0219c7e8(wk);
        break;
    case 2:
        switch (wk->state) {
        case 3:
            func_ov165_0219be4c(wk);
            break;
        case 4:
            func_ov165_0219c038(wk);
            break;
        case 22:
            func_ov165_0219c5d0(wk);
            break;
        case 23:
            func_ov165_0219bf40(wk);
            break;
        default:
            func_ov165_0219bc7c(wk);
            break;
        }
        break;
    }
}

static void func_ov165_0219bb68(PokeListWork *wk) {
    wk->cursorPos = wk->param->index;
    wk->subState = 0;
    wk->menuItem = 0x19;
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListMenu_SetButtonActive(wk->buttons[0], FALSE);
        PokeListMenu_SetButtonActive(wk->buttons[1], FALSE);
    }
    if (wk->touch == FALSE) {
        if (wk->cursorPos <= 5) {
            func_ov165_0219cdd8(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
        } else {
            PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
        }
    } else {
        func_0204c124(wk->cursor, FALSE);
        if (wk->cursorPos <= 5) {
            if (wk->state == 3 || wk->state == 22 || wk->state == 23) {
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
            } else {
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            }
        } else {
            PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], FALSE);
        }
    }
    func_0204c124(wk->shortcutButton, wk->showShortcutButtons);
    func_0204c124(wk->shortcutMark, wk->showShortcutButtons);
    func_0204c488(wk->shortcutMark, wk->param->keyItemRegistered + 6);
    if (PokeList_IsBattle(wk) == TRUE || func_ov165_0219da88(wk) == FALSE) {
        func_0204c124(wk->exitButton, FALSE);
    } else {
        func_0204c124(wk->exitButton, TRUE);
    }
}

static void func_ov165_0219bc7c(PokeListWork *wk) {
    u8 i;

    PokeListMessage_Close(wk, wk->message);
    if (PokeList_IsBattle(wk) == TRUE) {
        GFL_BGSysFillScrArea(0, 0, 0, 21, 32, 3, BGSYS_FILL_KEEP_PALETTE);
        for (i = 0; i < 6; i++) {
            wk->param->picked[i] = 0;
        }
        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            int entry = PokeListPlate_GetEntry(wk->plates[i]);

            if (entry <= 5) {
                wk->param->picked[entry] = i + 1;
            }
        }
    }
    switch (wk->input) {
    case 3:
        func_0204c124(wk->cursor, FALSE);
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        wk->param->index = wk->cursorPos;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        PokeList_SelectPokemon(wk);
        break;
    case 0:
        if (PokeList_IsBattle(wk) == TRUE) {
            func_ov165_0219c20c(wk);
        } else {
            wk->state = 19;
            wk->param->index = 6;
            wk->param->result = 0;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case 1:
        if (wk->cursorPos <= 5) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            func_0204c124(wk->cursor, FALSE);
        }
        if (PokeList_IsBattle(wk) == TRUE) {
            PokeListMenu_SetButtonActive(wk->buttons[0], FALSE);
            PokeListMenu_SetButtonPressed(wk->buttons[1], 1);
            wk->state = 12;
            wk->cursorPos = 7;
        } else {
            wk->state = 19;
            wk->pressedButton = wk->exitButton;
            func_0204c488(wk->pressedButton, 9);
        }
        wk->param->index = 7;
        wk->param->result = 0;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        break;
    case 2:
        if (wk->cursorPos <= 5) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            func_0204c124(wk->cursor, FALSE);
        }
        wk->state = 19;
        wk->param->index = 8;
        wk->param->result = 0;
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->pressedButton = wk->shortcutButton;
        func_0204c488(wk->pressedButton, 8);
        break;
    }
}

static void func_ov165_0219be4c(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    if (wk->input == 3 && wk->cursorPos == wk->selectPos) {
        wk->input = 1;
    }
    switch (wk->input) {
    case 3:
        PokeList_StartSwap(wk);
        break;
    case 1:
        wk->pkm = NULL;
        wk->state = 2;
        func_ov165_0219cdd8(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
        func_0204c124(wk->subCursor, FALSE);
        wk->selectPos = 9;
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        break;
    }
}

static int func_ov165_0219becc(PokeListWork *wk, u8 pos) {
    PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, pos);
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);

    if (PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL) != 0 || PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != 0) {
        return 5;
    }
    if (species == SPECIES_RESHIRAM) {
        if (hp == 0) {
            return 4;
        }
        return 0;
    }
    if (species == SPECIES_ZEKROM) {
        if (hp == 0) {
            return 4;
        }
        return 1;
    }
    return 3;
}

static void func_ov165_0219bf40(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    if (wk->input != 3 || wk->cursorPos == wk->selectPos) {
        wk->pkm = NULL;
        wk->state = 2;
        func_ov165_0219cdd8(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
        func_0204c124(wk->subCursor, FALSE);
        wk->selectPos = 9;
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else {
        int form;

        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        form = func_ov165_0219becc(wk, wk->cursorPos);
        if (form == 0 || form == 1) {
            wk->param->result = 10;
            wk->state = 14;
            wk->demo = 4;
            wk->demoForm = form;
        } else {
            PokeList_ShowMessage(wk, form + 0x52, TRUE, PokeList_MessageDoneSelect);
            func_ov165_0219cdd8(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            func_0204c124(wk->subCursor, FALSE);
            wk->param->index = wk->cursorPos;
        }
    }
}

// Shares a fifth of the user's HP with the Pokémon picked, as Soft-Boiled and Milk Drink do outside battle
static void func_ov165_0219c038(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    if (wk->input == 3) {
        u32 hp;
        u32 maxHp;

        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        hp = PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL);
        maxHp = PokeParty_GetParam(wk->pkm, PKM_PARAM_MAX_HP, NULL);
        if (wk->cursorPos == wk->selectPos2 || hp == 0 || maxHp == hp) {
            PokeList_ShowMessage(wk, 0x6e, TRUE, PokeList_MessageDoneSelect);
            func_ov165_0219cdd8(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos2], FALSE);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            func_0204c124(wk->subCursor, FALSE);
            wk->param->index = wk->cursorPos;
        } else {
            PartyPkm *user;
            u32 userMaxHp;
            u32 userHp;
            u32 amount;

            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            wk->pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
            wk->state = 10;
            wk->prevHp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);
            wk->hpDoneFunc = PokeList_ShareHpDone;
            func_ov165_0219cdd8(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos2], FALSE);
            func_0204c124(wk->subCursor, FALSE);
            user = PokeParty_GetPkm(wk->param->party, wk->selectPos2);
            userMaxHp = PokeParty_GetParam(user, PKM_PARAM_MAX_HP, NULL);
            userHp = PokeParty_GetParam(user, PKM_PARAM_HP, NULL);
            amount = userMaxHp / 5;
            if (hp + amount > maxHp) {
                PokeParty_SetParam(user, PKM_PARAM_HP, userHp - (maxHp - hp));
            } else {
                PokeParty_SetParam(user, PKM_PARAM_HP, userHp - amount);
            }
            wk->param->index = wk->cursorPos;
            wk->cursorPos = wk->selectPos2;
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            GFL_SndSEPlay(SEQ_SE_RECOVERY);
        }
    } else {
        wk->pkm = NULL;
        wk->state = 2;
        func_ov165_0219cdd8(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos2], FALSE);
        func_0204c124(wk->subCursor, FALSE);
        wk->selectPos2 = 9;
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
}

// Decides the team in a battle's selection, if the regulation allows it
static void func_ov165_0219c20c(PokeListWork *wk) {
    Regulation *regulation = wk->param->regulation;

    if (wk->cursorPos != 6) {
        if (wk->cursorPos <= 5) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            func_0204c124(wk->cursor, FALSE);
        }
        wk->cursorPos = 6;
    }
    wk->param->index = 6;
    {
        u8 picked[6] = { 0 };
        u8 i;
        u32 result;

        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            int entry = PokeListPlate_GetEntry(wk->plates[i]);

            if (entry <= 5) {
                picked[entry] = i + 1;
            }
        }
        result = func_0201f1e8(regulation, wk->param->party, picked);
        if (regulation->unk2 > wk->enteredCount) {
            PokeList_ShowMessage(wk, regulation->unk2 + 0x60, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (regulation->unk3 < wk->enteredCount) {
            PokeList_ShowMessage(wk, regulation->unk3 + 0x66, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (result == 1) {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetNumber(wk, wk->message, 0, regulation->unk6, 3);
            PokeList_ShowMessage(wk, 0x9e, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else if (result == 7) {
            PokeList_ShowMessage(wk, 0xa0, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            PokeListMenu_SetButtonActive(wk->buttons[1], FALSE);
            PokeListMenu_SetButtonPressed(wk->buttons[0], 1);
            wk->state = 12;
            wk->param->index = 6;
            wk->param->result = 0;
            wk->decided = TRUE;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
}

// Moves an item from one Pokémon to another, with its mail
static void func_ov165_0219c36c(PokeListWork *wk, PartyPkm *from, PartyPkm *to, u16 item) {
    if (PML_ItemIsMail(item) == FALSE) {
        PokeList_SetHeldItem(wk, from, 0);
        PokeList_SetHeldItem(wk, to, item);
    } else {
        MailData *mail = CreateMailData(wk->heapId);
        MailData *blank = CreateMailData(wk->heapId);

        PokeParty_GetParam(from, PKM_PARAM_MAIL, mail);
        PokeParty_SetParam(to, PKM_PARAM_MAIL, (u32)mail);
        PokeParty_SetParam(to, PKM_PARAM_ITEM, item);
        PokeParty_SetParam(from, PKM_PARAM_MAIL, (u32)blank);
        PokeParty_SetParam(from, PKM_PARAM_ITEM, 0);
        GFL_HeapFree(blank);
        GFL_HeapFree(mail);
    }
}

// Swaps the items of two Pokémon, with their mail
static void func_ov165_0219c3e8(PokeListWork *wk, PartyPkm *pkmA, PartyPkm *pkmB, u16 itemA, u16 itemB) {
    MailData *mailA = CreateMailData(wk->heapId);
    MailData *mailB = CreateMailData(wk->heapId);
    MailData *blank = CreateMailData(wk->heapId);

    if (PML_ItemIsMail(itemA) == FALSE && PML_ItemIsMail(itemB) == FALSE) {
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
    } else if (PML_ItemIsMail(itemA) == TRUE && PML_ItemIsMail(itemB) == FALSE) {
        PokeParty_GetParam(pkmA, PKM_PARAM_MAIL, mailA);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
        PokeParty_SetParam(pkmB, PKM_PARAM_MAIL, (u32)mailA);
        PokeParty_SetParam(pkmA, PKM_PARAM_MAIL, (u32)blank);
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
    } else if (PML_ItemIsMail(itemA) == FALSE && PML_ItemIsMail(itemB) == TRUE) {
        PokeParty_GetParam(pkmB, PKM_PARAM_MAIL, mailB);
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
        PokeParty_SetParam(pkmA, PKM_PARAM_MAIL, (u32)mailB);
        PokeParty_SetParam(pkmB, PKM_PARAM_MAIL, (u32)blank);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
    } else if (PML_ItemIsMail(itemA) == TRUE && PML_ItemIsMail(itemB) == TRUE) {
        PokeParty_GetParam(pkmA, PKM_PARAM_MAIL, mailA);
        PokeParty_GetParam(pkmB, PKM_PARAM_MAIL, mailB);
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
        PokeParty_SetParam(pkmA, PKM_PARAM_MAIL, (u32)mailB);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
        PokeParty_SetParam(pkmB, PKM_PARAM_MAIL, (u32)mailA);
    }
    GFL_HeapFree(blank);
    GFL_HeapFree(mailB);
    GFL_HeapFree(mailA);
}

// Updates the forms that two Pokémon's new items give them
static void func_ov165_0219c51c(PartyPkm *pkmA, PartyPkm *pkmB) {
    u16 items[2];
    PartyPkm *pkms[2];
    u16 i;

    pkms[0] = pkmA;
    pkms[1] = pkmB;
    items[0] = PokeParty_GetParam(pkmA, PKM_PARAM_ITEM, NULL);
    items[1] = PokeParty_GetParam(pkmB, PKM_PARAM_ITEM, NULL);
    for (i = 0; i < 2; i++) {
        // BUG: The first Pokémon's stats are calculated again each time, and the second's never are
#ifdef BUGFIX
        PokeParty_RecalcStats(pkms[i]);
#else
        PokeParty_RecalcStats(pkmA);
#endif
        PokeList_UpdateArceusForm(NULL, pkms[i], items[i]);
        PokeList_UpdateGenesectForm(NULL, pkms[i], items[i]);
    }
}

static void func_ov165_0219c578(PokeListWork *wk, u32 msgId) {
    GFL_SndSEPlay(SEQ_SE_BEEP);
    func_ov165_0219cdd8(wk, wk->cursorPos);
    PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
    PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
    func_0204c124(wk->subCursor, FALSE);
    PokeList_ShowMessage(wk, msgId, TRUE, PokeList_MessageDoneSelect);
}

// Gives the item of the Pokémon picked first to the one picked now, swapping their items
static void func_ov165_0219c5d0(PokeListWork *wk) {
    if (wk->input == 3) {
        PartyPkm *src = PokeParty_GetPkm(wk->param->party, wk->selectPos);
        PartyPkm *dst = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        u16 srcSpecies = PokeParty_GetParam(src, PKM_PARAM_LEGAL_SPECIES, NULL);
        u16 dstSpecies = PokeParty_GetParam(dst, PKM_PARAM_LEGAL_SPECIES, NULL);
        u16 srcItem = PokeParty_GetParam(src, PKM_PARAM_ITEM, NULL);
        u16 dstItem = PokeParty_GetParam(dst, PKM_PARAM_ITEM, NULL);

        PokeListMessage_Close(wk, wk->message);
        if ((dstSpecies == SPECIES_GIRATINA && srcItem == ITEM_GRISEOUS_ORB) ||
            (srcSpecies == SPECIES_GIRATINA && dstItem == ITEM_GRISEOUS_ORB)) {
            func_ov165_0219c578(wk, 0x5e);
        } else if (dstSpecies == SPECIES_GIRATINA && dstItem == ITEM_GRISEOUS_ORB) {
            func_ov165_0219c578(wk, 0x5f);
        } else if (dstSpecies == SPECIES_EGG) {
            func_ov165_0219c578(wk, 0x60);
        } else {
            if (dstItem == 0) {
                func_ov165_0219c36c(wk, src, dst, srcItem);
                PokeListMessage_SetPkmName(wk, wk->message, 0, dst);
            } else {
                func_ov165_0219c3e8(wk, src, dst, srcItem, dstItem);
                PokeListMessage_SetPkmName(wk, wk->message, 0, src);
                PokeListMessage_SetPkmName(wk, wk->message, 2, dst);
                PokeListMessage_SetItemName(wk, wk->message, 3, dstItem);
            }
            func_ov165_0219c51c(src, dst);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeListPlate_Redraw(wk, wk->plates[wk->selectPos]);
            func_ov165_0219cdd8(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            func_0204c124(wk->subCursor, FALSE);
            if (dstItem == 0) {
                PokeList_ShowMessage(wk, 0x5c, TRUE, PokeList_MessageDoneSelect);
            } else {
                PokeList_ShowMessage(wk, 0x5d, TRUE, PokeList_MessageDoneSelect);
            }
        }
        PokeListMessage_FreeWordSet(wk, wk->message);
    } else {
        PokeListMessage_Close(wk, wk->message);
        PokeListMessage_FreeWordSet(wk, wk->message);
        wk->pkm = NULL;
        wk->state = 2;
        func_ov165_0219cdd8(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
        PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
        func_0204c124(wk->subCursor, FALSE);
        // BUG: The selection of the Pokémon giving its item is kept, and that of Soft-Boiled's user is cleared instead
#ifdef BUGFIX
        wk->selectPos = 9;
#else
        wk->selectPos2 = 9;
#endif
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
}

static void func_ov165_0219c7e8(PokeListWork *wk) {
    func_ov165_0219c87c(wk);
    if (wk->input == 4) {
        func_ov165_0219cb80(wk);
    }
    if (wk->input != 4) {
        wk->subState = 2;
    }
    if (wk->param->unk73 == 1) {
        wk->subState = 2;
        wk->input = 1;
    }
}

// A was pressed
static void func_ov165_0219c820(PokeListWork *wk) {
    if (wk->cursorPos == 6) {
        wk->input = 0;
    } else if (wk->cursorPos == 7) {
        wk->input = 1;
    } else if (wk->unk28 == FALSE && PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
    } else if (wk->state == 22 && wk->cursorPos == wk->selectPos) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
    } else {
        wk->input = 3;
    }
}

// Moves the cursor with the keys, and takes A, B, X and Y
static void func_ov165_0219c87c(PokeListWork *wk) {
    static const u8 sUpFromButtonsTwo[] = { 0, 0, 2, 2, 4, 4 };
    static const u8 sUpFromButtons[] = { 0, 1, 1, 3, 3, 5 };
    u32 trg = GCTX_HIDGetPressedKeys();
    u32 rep = GCTX_HIDGetTypedKeys();

    if (wk->touch == TRUE && func_0204c138(wk->cursor) == FALSE) {
        if (trg != 0 && !(trg & (PAD_BUTTON_B | PAD_BUTTON_X))) {
            if (wk->cursorPos <= 5) {
                func_ov165_0219cdd8(wk, wk->cursorPos);
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
            } else {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                func_0204c124(wk->cursor, FALSE);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->touch = FALSE;
        }
    } else {
        s32 dir = 0;

        if (trg != 0) {
            wk->touch = FALSE;
        }
        if (rep & PAD_KEY_UP) {
            dir = -2;
        } else if (rep & PAD_KEY_DOWN) {
            dir = 2;
        } else if (rep & PAD_KEY_LEFT) {
            dir = -1;
        } else if (rep & PAD_KEY_RIGHT) {
            dir = 1;
        } else if (trg & PAD_BUTTON_A) {
            func_ov165_0219c820(wk);
        }
        if (dir != 0) {
            s32 prevPos = wk->cursorPos;
            BOOL done = FALSE;
            s32 last;

            if (PokeList_IsBattle(wk) == TRUE) {
                if (func_ov165_0219da74(wk) == TRUE) {
                    last = 7;
                } else {
                    last = 6;
                }
            } else {
                last = 5;
            }
            if ((wk->cursorPos == 6 || wk->cursorPos == 7) && (rep & (PAD_KEY_UP | PAD_KEY_DOWN))) {
                u8 count = PokeParty_GetPkmCount(wk->param->party);

                if (rep & PAD_KEY_UP) {
                    if (wk->cursorPos == 7 || func_ov165_0219da74(wk) == FALSE) {
                        wk->cursorPos = sUpFromButtons[count - 1];
                    } else {
                        wk->cursorPos = sUpFromButtonsTwo[count - 1];
                    }
                } else if (wk->cursorPos == 7 || func_ov165_0219da74(wk) == FALSE) {
                    if (count > 1) {
                        wk->cursorPos = 1;
                    } else {
                        wk->cursorPos = 0;
                    }
                } else {
                    wk->cursorPos = 0;
                }
            } else {
                do {
                    s32 next = wk->cursorPos + dir;

                    if (next > last) {
                        if (last == 6 && dir > 1 && prevPos != 6) {
                            wk->cursorPos = last;
                        } else {
                            wk->cursorPos = next - (last + 1);
                        }
                    } else if (next < 0) {
                        if (last == 6 && dir < 1) {
                            wk->cursorPos = last;
                        } else {
                            wk->cursorPos = wk->cursorPos + (last + 1) + dir;
                        }
                    } else {
                        wk->cursorPos += dir;
                    }
                    if (wk->cursorPos > 5 || PokeListPlate_IsValid(wk, wk->plates[wk->cursorPos]) == TRUE) {
                        done = TRUE;
                    }
                } while (done == FALSE);
            }
            if (prevPos != wk->cursorPos) {
                if (wk->cursorPos <= 5) {
                    func_ov165_0219cdd8(wk, wk->cursorPos);
                    PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
                } else {
                    PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                    func_0204c124(wk->cursor, FALSE);
                }
                if (prevPos <= 5) {
                    PokeListPlate_SetSelected(wk, wk->plates[prevPos], FALSE);
                } else {
                    PokeListMenu_SetButtonActive(wk->buttons[(u8)(prevPos - 6)], FALSE);
                }
                wk->flashTimer = 12;
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    }
    if (trg & PAD_BUTTON_B) {
        if ((PokeList_IsBattle(wk) == FALSE || func_ov165_0219da74(wk) == TRUE) && func_ov165_0219da88(wk) == TRUE) {
            wk->input = 1;
            wk->pressedButton = wk->exitButton;
            func_0204c488(wk->pressedButton, 9);
            wk->touch = FALSE;
        }
    } else if (trg & PAD_BUTTON_X) {
        if (wk->showShortcutButtons == TRUE) {
            wk->input = 2;
            wk->pressedButton = wk->shortcutButton;
            func_0204c488(wk->pressedButton, 8);
            wk->touch = FALSE;
        }
    } else if (trg & PAD_BUTTON_Y) {
        if (wk->showShortcutButtons == TRUE) {
            wk->param->keyItemRegistered ^= 1;
            func_0204c488(wk->shortcutMark, wk->param->keyItemRegistered + 6);
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            wk->touch = FALSE;
        }
    }
}

// Takes a touch of a plate or a button
static void func_ov165_0219cb80(PokeListWork *wk) {
    u8 slots[POKELIST_PLATE_COUNT];
    TouchRect plateRects[POKELIST_PLATE_COUNT + 1];
    u8 count = 0;
    u8 i;
    s32 hit;

    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        if (PokeListPlate_IsValid(wk, wk->plates[i]) == TRUE) {
            slots[count] = i;
            PokeListPlate_GetTouchRect(wk, wk->plates[i], &plateRects[count]);
            count++;
        }
    }
    plateRects[count].top = TOUCH_RECT_END;
    hit = func_0203da0c(plateRects);
    if (hit != TOUCH_RECT_NONE) {
        if (wk->unk28 == FALSE && hit <= 5 && PokeListPlate_IsEgg(wk, wk->plates[hit]) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (wk->state == 22 && hit <= 5 && hit == wk->selectPos) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            s32 prevPos;

            wk->input = 3;
            prevPos = wk->cursorPos;
            wk->cursorPos = slots[hit];
            if (prevPos <= 5) {
                PokeListPlate_SetSelected(wk, wk->plates[prevPos], FALSE);
            } else {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(prevPos - 6)], FALSE);
            }
            if (wk->cursorPos <= 5) {
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
            } else {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                func_0204c124(wk->cursor, FALSE);
            }
            wk->flashTimer = 12;
        }
        wk->touch = TRUE;
    }
    if (hit == TOUCH_RECT_NONE) {
        TouchRect buttonRects[4] = {
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { TOUCH_RECT_END, 0, 0, 0 },
        };

        if (PokeList_IsBattle(wk) == TRUE) {
            if (func_ov165_0219da74(wk) == TRUE) {
                buttonRects[1].top = 168;
                buttonRects[1].bottom = 192;
                buttonRects[1].left = 176;
                buttonRects[1].right = 255;
                buttonRects[0].top = 168;
                buttonRects[0].bottom = 192;
                buttonRects[0].left = 96;
                buttonRects[0].right = 176;
            } else {
                buttonRects[0].top = 168;
                buttonRects[0].bottom = 192;
                buttonRects[0].left = 176;
                buttonRects[0].right = 255;
            }
        } else {
            if (func_ov165_0219da88(wk) == TRUE) {
                buttonRects[1].top = 168;
                buttonRects[1].bottom = 192;
                buttonRects[1].left = 224;
                buttonRects[1].right = 248;
            }
            if (wk->showShortcutButtons == TRUE) {
                buttonRects[2].top = 168;
                buttonRects[2].bottom = 192;
                buttonRects[2].left = 208;
                buttonRects[2].right = 232;
            }
        }
        hit = func_0203da0c(buttonRects);
        if (hit != TOUCH_RECT_NONE) {
            wk->input = hit;
            wk->touch = TRUE;
            if (hit == 1) {
                func_0204c488(wk->exitButton, 9);
            }
        }
    }
    if (hit == TOUCH_RECT_NONE && wk->showShortcutButtons == TRUE) {
        TouchRect markRects[2] = {
            { 0, 0, 0, 0 },
            { TOUCH_RECT_END, 0, 0, 0 },
        };

        markRects[0].top = 168;
        markRects[0].bottom = 192;
        markRects[0].left = 184;
        markRects[0].right = 208;
        if (func_0203da0c(markRects) != TOUCH_RECT_NONE) {
            wk->param->keyItemRegistered ^= 1;
            func_0204c488(wk->shortcutMark, wk->param->keyItemRegistered + 6);
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            wk->touch = TRUE;
        }
    }
}

// Shows the cursor on a plate
static void func_ov165_0219cdd8(PokeListWork *wk, s32 pos) {
    ClActorPos actorPos;

    PokeListPlate_GetCursorPos(wk, wk->plates[pos], &actorPos);
    func_0204c140(wk->cursor, &actorPos, 0);
    func_0204c124(wk->cursor, TRUE);
    func_0204c468(wk->cursor, 3);
    if (pos == 0) {
        func_0204c488(wk->cursor, 0);
    } else {
        func_0204c488(wk->cursor, 1);
    }
}

// Shows the second cursor on a plate, which marks the Pokémon picked first
static void func_ov165_0219ce34(PokeListWork *wk, s32 pos) {
    ClActorPos actorPos;

    PokeListPlate_GetCursorPos(wk, wk->plates[pos], &actorPos);
    func_0204c140(wk->subCursor, &actorPos, 0);
    func_0204c124(wk->subCursor, TRUE);
    func_0204c468(wk->subCursor, 2);
    if (pos == 0) {
        func_0204c488(wk->subCursor, 2);
    } else {
        func_0204c488(wk->subCursor, 3);
    }
}

static void PokeList_UpdateMenu(PokeListWork *wk) {
    switch (wk->subState) {
    case 0:
        func_ov165_0219ceec(wk);
        wk->subState = 1;
        break;
    case 1: {
        u32 item;

        PokeListMenu_Update(wk, wk->menu);
        item = PokeListMenu_GetPicked(wk, wk->menu);
        if (item != 0x19) {
            wk->menuItem = item;
            wk->subState = 2;
        }
        if (wk->param->unk73 == 1) {
            wk->subState = 2;
        }
        break;
    }
    case 2:
        func_ov165_0219cf70(wk);
        PokeList_DoMenuItem(wk);
        break;
    }
}

// Dims the screen but the menu, for the menu to open
static void func_ov165_0219ceec(PokeListWork *wk) {
    wk->state = 6;
    wk->subState = 0;
    gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                             8);
    G2_SetWnd0Position(224, 168, 248, 192);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          FALSE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          TRUE);
    wk->glowPaused = TRUE;
    func_0204c124(wk->shortcutButton, FALSE);
    func_0204c124(wk->exitButton, FALSE);
    func_0204c124(wk->shortcutMark, FALSE);
    GFL_BGSysQueueScrLoad(0);
    func_0203d564(wk->touch);
}

static void func_ov165_0219cf70(PokeListWork *wk) {
    PokeListMenu_Close(wk, wk->menu);
    PokeListMessage_Close(wk, wk->message);
    if (PokeList_IsBattle(wk) == TRUE || func_ov165_0219da88(wk) == FALSE) {
        func_0204c124(wk->exitButton, FALSE);
    } else {
        func_0204c124(wk->exitButton, TRUE);
    }
    if (wk->menuItem != 4 && wk->menuItem != 5) {
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, GX_PLANEMASK_BG3, 16, 10);
        wk->glowPaused = FALSE;
    }
    GFL_BGSysFillScrAsync(0, 0);
    wk->touch = func_0203d554();
}

// The moves outside battle, by the index of their check in CheckAllowHidenEvent
static const u16 sHidenMoves[] = {
    MOVE_CUT, MOVE_SURF,        MOVE_WATERFALL, MOVE_STRENGTH, MOVE_FLY,        MOVE_FLASH,      MOVE_TELEPORT,
    MOVE_DIG, MOVE_SWEET_SCENT, MOVE_CHATTER,   MOVE_DIVE,     MOVE_MILK_DRINK, MOVE_SOFTBOILED,
};

// Does what the menu item picked does
static void PokeList_DoMenuItem(PokeListWork *wk) {
    if (wk->param->unk73 == 1) {
        wk->state = 19;
        return;
    }
    switch (wk->menuItem) {
    case 6:
        func_ov165_0219cdd8(wk, wk->cursorPos);
        wk->pkm = NULL;
        wk->state = 2;
        PokeList_ShowModeMessage(wk);
        break;
    case 3:
        func_ov165_0219cdd8(wk, wk->cursorPos);
        func_ov165_0219ce34(wk, wk->cursorPos);
        wk->selectPos = wk->cursorPos;
        wk->state = 3;
        wk->pkm = NULL;
        wk->showShortcutButtons = FALSE;
        PokeListMessage_Open(wk, wk->message, 0);
        PokeListMessage_Print(wk, wk->message, 0xb);
        func_ov165_0219bb68(wk);
        break;
    case 4: {
        PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        u16 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
        u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        u32 itemMenus[2][5] = {
            { 9, 10, 22, 6, 16 },
            { 9, 10, 6, 16 },
        };

        func_ov165_0219ceec(wk);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, 0x14);
        if (item == ITEM_GRISEOUS_ORB && species == SPECIES_GIRATINA) {
            PokeListMenu_Open(wk, wk->menu, itemMenus[1]);
        } else {
            PokeListMenu_Open(wk, wk->menu, itemMenus[0]);
        }
        GFL_BGSysLoadScr(0);
        break;
    }
    case 5: {
        u32 mailMenu[5] = { 12, 13, 23, 6, 16 };

        func_ov165_0219ceec(wk);
        PokeListMessage_Open(wk, wk->message, 2);
        PokeListMessage_Print(wk, wk->message, 0x14);
        PokeListMenu_Open(wk, wk->menu, mailMenu);
        GFL_BGSysLoadScr(0);
        break;
    }
    case 0:
        wk->state = 19;
        wk->param->index = wk->cursorPos;
        wk->param->result = 1;
        break;
    case 20: {
        Regulation *regulation = wk->param->regulation;
        BOOL full = FALSE;
        u32 result = PokeListPlate_CheckEntry(wk, wk->plates[wk->cursorPos]);

        if (result == 0) {
            PokeListPlate_SetEntry(wk, wk->plates[wk->cursorPos], wk->enteredCount);
            wk->enteredCount++;
            PokeList_ShowModeMessage(wk);
            if (wk->enteredCount == regulation->unk3) {
                full = TRUE;
            }
            wk->state = 2;
        } else if (result == 2) {
            PokeList_ShowMessage(wk, 0x9c, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (result == 3) {
            PokeList_ShowMessage(wk, 0x9d, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (result == 4) {
            PokeList_ShowMessage(wk, regulation->unk3 + 0x66, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        wk->pkm = NULL;
        if (full == TRUE) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            wk->cursorPos = 6;
            if (wk->touch == FALSE) {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                func_0204c124(wk->cursor, FALSE);
            }
        } else if (wk->touch == FALSE) {
            func_ov165_0219cdd8(wk, wk->cursorPos);
        }
        break;
    }
    case 21: {
        int entry = PokeListPlate_GetEntry(wk->plates[wk->cursorPos]);
        u8 i;

        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            int other = PokeListPlate_GetEntry(wk->plates[i]);

            if (other <= 5 && other > entry) {
                PokeListPlate_SetEntry(wk, wk->plates[i], other - 1);
            }
        }
        PokeListPlate_SetEntry(wk, wk->plates[wk->cursorPos], 6);
        wk->enteredCount--;
        if (wk->touch == FALSE) {
            func_ov165_0219cdd8(wk, wk->cursorPos);
        }
        wk->pkm = NULL;
        wk->state = 2;
        PokeList_ShowModeMessage(wk);
        break;
    }
    case 10: {
        u32 item = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        PokeList_UpdateArceusForm(wk, wk->pkm, 0);
        PokeList_UpdateGenesectForm(wk, wk->pkm, 0);
        if (item == 0) {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeList_ShowMessage(wk, 0x3e, TRUE, PokeList_MessageDoneSelect);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else if (PokeList_GetBagCount(wk, item) == 999) {
            PokeList_ShowMessage(wk, 0x40, TRUE, PokeList_MessageDoneSelect);
        } else {
            PokeList_SetHeldItem(wk, wk->pkm, 0);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, item);
            if (PokeListDemo_CanBecomeAltered(wk, wk->pkm) == TRUE) {
                PokeListDemo_SetAltered(wk, wk->pkm);
                PokeList_ShowMessage(wk, 0x3f, TRUE, PokeList_MessageDoneDemo);
                wk->demo = 2;
            } else {
                PokeList_ShowMessage(wk, 0x3f, TRUE, PokeList_MessageDoneSelect);
            }
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    }
    case 22:
    case 23: {
        u32 item = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        PokeList_UpdateArceusForm(wk, wk->pkm, 0);
        PokeList_UpdateGenesectForm(wk, wk->pkm, 0);
        PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
        if (item == 0) {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeList_ShowMessage(wk, 0x3e, TRUE, PokeList_MessageDoneSelect);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, item);
            func_ov165_0219cdd8(wk, wk->cursorPos);
            func_ov165_0219ce34(wk, wk->cursorPos);
            wk->selectPos = wk->cursorPos;
            wk->state = 22;
            wk->pkm = NULL;
            wk->showShortcutButtons = FALSE;
            PokeListMessage_Open(wk, wk->message, 0);
            PokeListMessage_Print(wk, wk->message, 0xb);
            func_ov165_0219bb68(wk);
        }
        break;
    }
    case 11:
        wk->state = 19;
        wk->param->index = wk->cursorPos;
        wk->param->result = 0;
        break;
    case 9:
        wk->state = 19;
        wk->param->index = wk->cursorPos;
        wk->param->result = 3;
        break;
    case 12:
        wk->state = 19;
        wk->param->index = wk->cursorPos;
        wk->param->result = 7;
        break;
    case 13:
        PokeList_ShowMessage(wk, 0x18, FALSE, PokeList_AskTakeMail);
        break;
    case 7:
        wk->state = 19;
        wk->param->index = wk->cursorPos;
        wk->param->result = 0;
        break;
    case 16:
    case 17:
    case 18:
    case 19:
        if (wk->param->mode == 5) {
            if (StatusRcv_CanUseItem(wk->pkm, wk->param->item, wk->menuItem - 16, wk->heapId) == TRUE) {
                u32 move = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + wk->menuItem - 16, NULL);

                GFL_BGSysLoadScr(0);
                PokeList_ShowItemResult(wk, move);
                StatusRcv_UseItem(wk->pkm, wk->param->item, wk->menuItem - 16, wk->param->zoneId, wk->heapId);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                PokeList_SubFromBag(wk, wk->param->item);
                GFL_SndSEPlay(SEQ_SE_RECOVERY);
            } else {
                PokeList_ShowItemUseless(wk);
            }
        } else {
            u32 hiden = PokeList_GetHidenResult(wk->pkm, wk->menuItem - 16);

            if (hiden == 22 || hiden == 23) {
                if (func_02018c38(wk->param->zoneId) == FALSE) {
                    PokeList_ShowMessage(wk, 0x51, TRUE, PokeList_MessageDoneSelect);
                } else {
                    u32 maxHp = PokeParty_GetParam(wk->pkm, PKM_PARAM_MAX_HP, NULL);
                    u32 hp = PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL);

                    if (hp > maxHp / 5) {
                        func_ov165_0219cdd8(wk, wk->cursorPos);
                        func_ov165_0219ce34(wk, wk->cursorPos);
                        wk->selectPos2 = wk->cursorPos;
                        wk->state = 4;
                        wk->pkm = NULL;
                        wk->showShortcutButtons = FALSE;
                        PokeListMessage_Open(wk, wk->message, 0);
                        PokeListMessage_Print(wk, wk->message, 0x11);
                        func_ov165_0219bb68(wk);
                    } else {
                        PokeList_ShowMessage(wk, 0x75, TRUE, PokeList_MessageDoneSelect);
                    }
                }
            } else {
                switch (CheckAllowHidenEvent(hiden - 11, &wk->param->action)) {
                case 0:
                    wk->state = 19;
                    wk->param->index = wk->cursorPos;
                    wk->param->result = hiden;
                    break;
                case 1:
                    PokeList_ShowMessage(wk, 0x51, TRUE, PokeList_MessageDoneSelect);
                    break;
                case 2:
                    PokeList_ShowMessage(wk, 0x39, TRUE, PokeList_MessageDoneSelect);
                    break;
                case 3:
                    PokeListMessage_CreateWordSet(wk, wk->message);
                    PokeListMessage_SetMoveName(wk, wk->message, 0, sHidenMoves[hiden - 11]);
                    PokeList_ShowMessage(wk, 0xab, TRUE, PokeList_MessageDoneSelect);
                    PokeListMessage_FreeWordSet(wk, wk->message);
                    break;
                case 4:
                    PokeList_ShowMessage(wk, 0x50, TRUE, PokeList_MessageDoneSelect);
                    break;
                }
            }
        }
        break;
    default:
        func_ov165_0219cdd8(wk, wk->cursorPos);
        wk->pkm = NULL;
        wk->state = 2;
        PokeList_ShowModeMessage(wk);
        break;
    }
}

// Waits for the message to be read
static void PokeList_WaitMessage(PokeListWork *wk) {
    if (PokeListMessage_IsDone(wk, wk->message) == TRUE) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            wk->touch = FALSE;
        }
        if (func_0203da48() == TRUE) {
            wk->touch = TRUE;
        }
        wk->msgDoneFunc(wk);
    }
}

void PokeList_ShowMessage(PokeListWork *wk, u32 msgId, BOOL waitInput, void (*doneFunc)(PokeListWork *wk)) {
    wk->state = 7;
    wk->msgWaitInput = waitInput;
    wk->msgDoneFunc = doneFunc;
    PokeListMessage_Open(wk, wk->message, 3);
    PokeListMessage_PrintStream(wk, wk->message, msgId, waitInput);
    func_0204c124(wk->exitButton, FALSE);
    func_0204c124(wk->shortcutButton, FALSE);
    func_0204c124(wk->shortcutMark, FALSE);
}

static void PokeList_UpdateSubMenu(PokeListWork *wk) {
    u32 item;

    PokeListMenu_Update(wk, wk->menu);
    item = PokeListMenu_GetPicked(wk, wk->menu);
    if (item != 0x19) {
        wk->menuFunc(wk, item);
        PokeListMenu_Close(wk, wk->menu);
    }
}

static void PokeList_OpenSubMenu(PokeListWork *wk, void (*func)(PokeListWork *wk, u32 item)) {
    wk->state = 9;
    wk->menuFunc = func;
    PokeListMenu_OpenYesNo(wk, wk->menu);
}

// Slides the plates of the two Pokémon being swapped out, swaps them, and slides them back in
static void PokeList_UpdateSwap(PokeListWork *wk) {
    switch (wk->subState) {
    case 3:
        PokeListPlate_ClearSlid(wk, wk->plates[wk->cursorPos], wk->unk108);
        PokeListPlate_ClearSlid(wk, wk->plates[wk->selectPos], wk->unk108);
        wk->unk108++;
        PokeListPlate_DrawSlid(wk, wk->plates[wk->cursorPos], wk->unk108);
        PokeListPlate_DrawSlid(wk, wk->plates[wk->selectPos], wk->unk108);
        if (wk->unk108 >= 16) {
            PokeParty_SwapPkms(wk->param->party, wk->cursorPos, wk->selectPos, wk->heapId);
            PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], PokeParty_GetPkm(wk->param->party, wk->cursorPos),
                                 wk->unk108);
            PokeListPlate_SetPkm(wk, wk->plates[wk->selectPos], PokeParty_GetPkm(wk->param->party, wk->selectPos),
                                 wk->unk108);
            wk->subState = 4;
            GFL_SndSEPlay(SEQ_SE_SYS_03);
        }
        break;
    case 4:
        PokeListPlate_ClearSlid(wk, wk->plates[wk->cursorPos], wk->unk108);
        PokeListPlate_ClearSlid(wk, wk->plates[wk->selectPos], wk->unk108);
        wk->unk108--;
        PokeListPlate_DrawSlid(wk, wk->plates[wk->cursorPos], wk->unk108);
        PokeListPlate_DrawSlid(wk, wk->plates[wk->selectPos], wk->unk108);
        if (wk->unk108 == 0) {
            PokeList_EndSwap(wk);
        }
        break;
    }
}

static void PokeList_StartSwap(PokeListWork *wk) {
    wk->state = 5;
    wk->subState = 3;
    wk->unk108 = 0;
    GFL_SndSEPlay(SEQ_SE_SYS_02);
    func_0204c124(wk->cursor, FALSE);
    func_0204c124(wk->subCursor, FALSE);
}

static void PokeList_EndSwap(PokeListWork *wk) {
    wk->pkm = NULL;
    wk->state = 2;
    func_ov165_0219cdd8(wk, wk->cursorPos);
    PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
    func_0204c124(wk->subCursor, FALSE);
    wk->selectPos = 9;
    wk->param->index = wk->cursorPos;
    PokeList_ShowModeMessage(wk);
}

// The result that a move used outside battle ends the list with, or 0 for a move that can't be
u32 PokeList_GetHidenResult(PartyPkm *pkm, u8 slot) {
    switch (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL)) {
    case MOVE_CUT:
        return 11;
    case MOVE_FLY:
        return 15;
    case MOVE_SURF:
        return 12;
    case MOVE_STRENGTH:
        return 14;
    case MOVE_WATERFALL:
        return 13;
    case MOVE_FLASH:
        return 16;
    case MOVE_TELEPORT:
        return 17;
    case MOVE_DIG:
        return 18;
    case MOVE_SWEET_SCENT:
        return 19;
    case MOVE_CHATTER:
        return 20;
    case MOVE_DIVE:
        return 21;
    case MOVE_MILK_DRINK:
        return 22;
    case MOVE_SOFTBOILED:
        return 23;
    }
    return 0;
}

BOOL PokeList_IsBattle(PokeListWork *wk) {
    if (wk->param->mode == 1 || wk->param->mode == 22 || wk->param->mode == 23) {
        return TRUE;
    }
    return FALSE;
}

// Whether the cancel button is shown next to the battle's decide button
static BOOL func_ov165_0219da74(PokeListWork *wk) {
    if (wk->param->mode == 1) {
        return FALSE;
    }
    return TRUE;
}

static BOOL func_ov165_0219da88(PokeListWork *wk) {
    if (wk->param->mode == 21) {
        return FALSE;
    }
    return TRUE;
}

// Whether a Pokémon can learn the move to teach: 0 into an empty slot, 1 in place of a move, 2 not at all, and 3 if
// it knows it already
u32 PokeList_CheckLearnMove(PokeListWork *wk, PartyPkm *pkm, u8 pos) {
    u8 i;
    BOOL hasEmptySlot = FALSE;
    u32 ret;

    if (wk->wasMode18 == TRUE) {
        u32 moveA = wk->param->move;
        u32 moveB;
        u32 moveC;

        if (moveA == POKELIST_MOVE_ULTIMATE) {
            moveA = MOVE_FRENZY_PLANT;
            moveB = MOVE_BLAST_BURN;
            moveC = MOVE_HYDRO_CANNON;
        } else if (moveA == POKELIST_MOVE_PLEDGE) {
            moveA = MOVE_GRASS_PLEDGE;
            moveB = MOVE_FIRE_PLEDGE;
            moveC = MOVE_WATER_PLEDGE;
        } else {
            moveB = moveA;
            moveC = moveA;
        }
        for (i = 0; i < 4; i++) {
            u32 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

            if (move == moveA || move == moveB || move == moveC) {
                return 3;
            }
            if (move == 0) {
                hasEmptySlot = TRUE;
            }
        }
        if (wk->param->unk6E & (1 << pos)) {
            if (hasEmptySlot == TRUE) {
                return 0;
            }
            return 1;
        }
        return 2;
    }
    for (i = 0; i < 4; i++) {
        u32 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

        if (move == wk->param->move) {
            return 3;
        }
        if (move == 0) {
            hasEmptySlot = TRUE;
        }
    }
    if (wk->param->item != 0) {
        u8 tm = PML_ItemGetTMBitMask(wk->param->item);

        if (tm != 0xff && canPkmLearnTM_Wrapper(pkm, tm) == TRUE) {
            ret = 0;
            if (hasEmptySlot != TRUE) {
                ret = 1;
            }
            return ret;
        }
        return 2;
    }
    ret = 0;
    if (hasEmptySlot != TRUE) {
        ret = 1;
    }
    return ret;
}

BOOL PokeList_CanEvolveWithItem(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    return CheckEvolveSpecies(wk->param->party, pkm, 3, item, wk->param->season, NULL, wk->heapId);
}

void PokeList_PrintString(PokeListWork *wk, BmpWin *window, u16 msgId, u16 x, s16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(window), x, y, str, wk->font, color);
    GFL_StrBufFree(str);
}

void PokeList_PrintStringSmall(PokeListWork *wk, BmpWin *window, u32 msgId, int x, s16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(window), x, y, str, wk->smallFont, color);
    GFL_StrBufFree(str);
}

// Draws a string at once, without the print queue
void PokeList_DrawStringSmall(PokeListWork *wk, BmpWin *window, u32 msgId, int x, s16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), x, y, str, wk->smallFont, color);
    GFL_StrBufFree(str);
}

void PokeList_PrintWordSetString(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                 u16 color) {
    StrBuf *buf = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, buf, str);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(window), x, y, buf, wk->font, color);
    GFL_StrBufFree(str);
    GFL_StrBufFree(buf);
}

void PokeList_PrintWordSetStringSmall(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                      u16 color) {
    StrBuf *buf = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, buf, str);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(window), x, y, buf, wk->smallFont, color);
    GFL_StrBufFree(str);
    GFL_StrBufFree(buf);
}

void PokeList_DrawWordSetStringSmall(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                     u16 color) {
    StrBuf *buf = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, buf, str);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), x, y, buf, wk->smallFont, color);
    GFL_StrBufFree(str);
    GFL_StrBufFree(buf);
}

// Called once a message is read: back to picking a Pokémon
void PokeList_MessageDoneSelect(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    wk->state = 2;
    PokeList_ShowModeMessage(wk);
}

// Called once a message is read: the list ends
void PokeList_MessageDoneExit(PokeListWork *wk) {
    wk->state = 19;
}

// Called once a message about using an item is read: back to picking a Pokémon, if there are items left
void PokeList_MessageDoneItem(PokeListWork *wk) {
    if (PokeList_GetBagCount(wk, wk->param->item)) {
        PokeListMessage_Close(wk, wk->message);
        wk->state = 2;
        PokeList_ShowModeMessage(wk);
    } else {
        PokeListMessage_Close(wk, wk->message);
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetItemName(wk, wk->message, 0, wk->param->item);
        PokeList_ShowMessage(wk, 0xbf, TRUE, PokeList_MessageDoneExit);
        PokeListMessage_FreeWordSet(wk, wk->message);
    }
}

// Asks whether to give up learning the move
void PokeList_AskStopLearning(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerStopLearning);
}

static void PokeList_AnswerStopLearning(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == 14) {
        if (wk->param->mode == 5 || wk->param->mode == 8) {
            wk->param->result = 5;
        } else {
            wk->param->result = 4;
        }
        PokeList_ShowMessage(wk, 0x28, TRUE, PokeList_MessageDoneExit);
    } else {
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
        PokeList_ShowMessage(wk, 0x24, FALSE, PokeList_AskForgetMove);
        PokeListMessage_FreeWordSet(wk, wk->message);
    }
}

// Asks whether to forget a move for the new one
static void PokeList_AskForgetMove(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerForgetMove);
}

static void PokeList_AnswerForgetMove(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == 14) {
        wk->param->result = 10;
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
        if (wk->param->mode == 5 || wk->param->mode == 8) {
            PokeList_ShowMessage(wk, 0x27, TRUE, PokeList_LearnMessageDone);
        } else {
            PokeList_ShowMessage(wk, 0x27, TRUE, PokeList_MessageDoneExit);
        }
        PokeListMessage_FreeWordSet(wk, wk->message);
    } else {
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
        PokeList_ShowMessage(wk, 0x21, FALSE, PokeList_AskStopLearning);
        PokeListMessage_FreeWordSet(wk, wk->message);
    }
}

// The move was forgotten: the new one takes its slot
static void PokeList_LearnInSlot(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
    if (wk->param->mode == 5 || wk->param->mode == 8) {
        PokeList_ShowMessage(wk, 0x2a, TRUE, PokeList_LearnMessageDone);
    } else {
        PokeList_ShowMessage(wk, 0x2a, TRUE, PokeList_MessageDoneExit);
        PokeList_RaiseFriendship(wk, wk->pkm);
        wk->param->result = 10;
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
    PokeList_SetMove(wk, wk->pkm, wk->param->moveSlot);
}

// Asks whether to swap the item held for the one given
static void PokeList_AskSwapItem(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerSwapItem);
}

static void PokeList_AnswerSwapItem(PokeListWork *wk, u32 item) {
    if (item == 14) {
        u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        if (PokeList_GetBagCount(wk, heldItem) == 999) {
            if (wk->param->mode == 10) {
                wk->param->mode = 0;
                PokeListMessage_Close(wk, wk->message);
                PokeList_ShowMessage(wk, 0x40, TRUE, PokeList_MessageDoneSelect);
            } else {
                wk->state = 19;
                PokeListMessage_Close(wk, wk->message);
                PokeList_ShowMessage(wk, 0x40, TRUE, PokeList_MessageDoneExit);
            }
        } else if (PML_ItemIsMail(wk->param->item) == TRUE) {
            wk->param->index = wk->cursorPos;
            wk->state = 19;
            wk->param->result = 6;
        } else {
            BOOL griseousForm;
            BOOL plateForm;

            PokeListMessage_Close(wk, wk->message);
            PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
            PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
            griseousForm = PokeListDemo_CanBecomeOrigin(wk, wk->pkm);
            plateForm = PokeListDemo_CanBecomeAltered(wk, wk->pkm);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetItemName(wk, wk->message, 1, heldItem);
            PokeListMessage_SetItemName(wk, wk->message, 2, wk->param->item);
            if (wk->param->mode == 10) {
                wk->param->mode = 0;
                if (griseousForm == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, 0x41, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = 1;
                } else if (plateForm == TRUE) {
                    PokeListDemo_SetAltered(wk, wk->pkm);
                    PokeList_ShowMessage(wk, 0x41, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = 2;
                } else {
                    PokeList_ShowMessage(wk, 0x41, TRUE, PokeList_MessageDoneSelect);
                }
            } else {
                wk->param->result = 10;
                if (griseousForm == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, 0x41, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = 1;
                } else if (plateForm == TRUE) {
                    PokeListDemo_SetAltered(wk, wk->pkm);
                    PokeList_ShowMessage(wk, 0x41, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = 2;
                } else {
                    PokeList_ShowMessage(wk, 0x41, TRUE, PokeList_MessageDoneExit);
                }
            }
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
    } else if (wk->param->mode == 10) {
        wk->param->mode = 0;
        PokeListMessage_Close(wk, wk->message);
        wk->state = 2;
        PokeList_ShowModeMessage(wk);
    } else {
        wk->state = 19;
    }
}

// The user's HP has gone down: the HP it shares is given to the Pokémon picked, whose HP goes up next
static void PokeList_ShareHpDone(PokeListWork *wk) {
    PartyPkm *user = PokeParty_GetPkm(wk->param->party, wk->selectPos2);
    // Read but not used
    u32 userHp = PokeParty_GetParam(user, PKM_PARAM_HP, NULL);
    u32 userMaxHp = PokeParty_GetParam(user, PKM_PARAM_MAX_HP, NULL);
    PartyPkm *target = PokeParty_GetPkm(wk->param->party, wk->param->index);
    u32 hp = PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL);
    u32 maxHp = PokeParty_GetParam(wk->pkm, PKM_PARAM_MAX_HP, NULL);

    wk->cursorPos = wk->param->index;
    if (hp + userMaxHp / 5 > maxHp) {
        PokeParty_SetParam(target, PKM_PARAM_HP, maxHp);
    } else {
        PokeParty_SetParam(target, PKM_PARAM_HP, hp + userMaxHp / 5);
    }
    wk->hpDoneFunc = PokeList_ShareHpEnd;
    wk->state = 10;
    wk->playHealSe = TRUE;
}

static void PokeList_ShareHpEnd(PokeListWork *wk) {
    u16 hp = PokeListPlate_GetHp(wk, wk->plates[wk->param->index]);

    wk->cursorPos = wk->param->index;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    if (wk->prevHp != 0) {
        PokeListMessage_SetNumber(wk, wk->message, 1, hp - wk->prevHp, 3);
        PokeList_ShowMessage(wk, 0x2d, TRUE, PokeList_MessageDoneSelect);
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
}

// Asks whether to take the mail
static void PokeList_AskTakeMail(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerTakeMail);
}

static void PokeList_AnswerTakeMail(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == 14) {
        s32 slot = func_020097c4(wk->param->unk08, 0);

        if (slot >= 0) {
            MailData *mail = CreateMailData(wk->heapId);

            PokeParty_GetParam(wk->pkm, PKM_PARAM_MAIL, mail);
            func_020097e0(wk->param->unk08, 0, slot, mail);
            GFL_HeapFree(mail);
            PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, 0);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeList_ShowMessage(wk, 0x1b, TRUE, PokeList_MessageDoneSelect);
        } else {
            PokeList_ShowMessage(wk, 0x1f, TRUE, PokeList_MessageDoneSelect);
        }
    } else {
        PokeList_ShowMessage(wk, 0x1c, FALSE, PokeList_AskDeleteMail);
    }
}

// Asks whether to take the mail anyway, which loses its message
static void PokeList_AskDeleteMail(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerDeleteMail);
}

static void PokeList_AnswerDeleteMail(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == 14) {
        u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        if (PokeList_GetBagCount(wk, heldItem) == 999) {
            PokeList_ShowMessage(wk, 0x40, TRUE, PokeList_MessageDoneSelect);
        } else {
            PokeList_AddToBag(wk, heldItem);
            PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, 0);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeList_ShowMessage(wk, 0x20, TRUE, PokeList_MessageDoneSelect);
        }
    } else {
        PokeListMessage_Close(wk, wk->message);
        wk->state = 2;
        PokeList_ShowModeMessage(wk);
    }
}

// Called once a message is read: the form change is shown
static void PokeList_MessageDoneDemo(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    wk->state = 14;
}

// The time to pick a team ran out
void PokeList_TimeUp(PokeListWork *wk) {
    if (wk->timeUp == FALSE && wk->state != 19 && wk->state != 21 && wk->state != 0 && wk->state != 1) {
        if (wk->state == 6) {
            func_ov165_0219cf70(wk);
        }
        wk->timeUp = TRUE;
        wk->state = wk->unk284 == TRUE ? 17 : 20;
        wk->param->index = 6;
        wk->param->result = 0;
        if (wk->decided == FALSE) {
            Regulation *regulation;
            u8 i;

            for (i = 0; i < 6; i++) {
                wk->param->picked[i] = 0;
            }
            regulation = wk->param->regulation;
            if (func_0201f97c(regulation, wk->param->party, wk->param->picked) == 0) {
                for (i = 0; i < 6; i++) {
                    wk->param->picked[i] = 0;
                }
                for (i = 0; i < regulation->unk2; i++) {
                    wk->param->picked[i] = i + 1;
                }
            }
        }
    }
}

static void PokeList_LearnMove(PokeListWork *wk, PartyPkm *pkm) {
    if (wk->param->mode == 5 || wk->param->mode == 8) {
        PokeParty_LearnMove(pkm, wk->param->move);
    } else {
        func_0201d268(pkm, wk->param->move);
    }
}

static void PokeList_SetMove(PokeListWork *wk, PartyPkm *pkm, u8 slot) {
    if (wk->param->mode == 5 || wk->param->mode == 8) {
        PokeParty_SetMove(pkm, wk->param->move, slot);
    } else {
        func_0201d2d0(pkm, wk->param->move, slot);
    }
}

static void PokeList_RaiseFriendship(PokeListWork *wk, PartyPkm *pkm) {
    FriendshipManagerCalc(pkm, 7, wk->param->zoneId, wk->heapId);
}

// Gives a Pokémon an item from the bag, putting the item it held back in the bag
static void PokeList_SetHeldItem(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    u32 heldItem = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);

    if (heldItem != 0) {
        PokeList_AddToBag(wk, heldItem);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, item);
    if (item != 0) {
        PokeList_SubFromBag(wk, item);
    }
}

static u16 PokeList_GetBagCount(PokeListWork *wk, u16 item) {
    return BagSave_GetItemCountByID(wk->param->bag, item, wk->heapId);
}

static void PokeList_SubFromBag(PokeListWork *wk, u16 item) {
    BagSave_SubItem(wk->param->bag, item, 1, wk->heapId);
}

static void PokeList_AddToBag(PokeListWork *wk, u16 item) {
    BagSave_AddItem(wk->param->bag, item, 1, wk->heapId);
}
