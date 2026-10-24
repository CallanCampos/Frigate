#include "types.h"
#include "app/pokemon_trade/pokemontrade_2d.h"
#include "app/pokemon_trade/pokemontrade_3d.h"
#include "app/pokemon_trade/pokemontrade_message.h"
#include "app/pokemon_trade/pokemontrade_nego.h"
#include "app/pokemon_trade/pokemontrade_proc.h"
#include "app/pokemon_trade/pokemontrade_save.h"
#include "app/pokemon_trade/pokemontrade_touchbar.h"
#include "app/pokemon_trade_local.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "field/unity_tower.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/nhttp_rap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "pml/hm_check.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/mcss.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The trade's procs and the steps of the trade: choosing a Pokémon, the menus, the messages between the machines

static BOOL PokemonTrade_IsMailBoxFull(PokemonTradeWork *wk);
static void PokemonTrade_UpdateBGM(PokemonTradeWork *wk);
static void PokemonTrade_RequestBoxRedraw(PokemonTradeWork *wk);
static void PokemonTrade_UpdateDragState(PokemonTradeWork *wk);
static void PokemonTrade_UpdateDragIcon(PokemonTradeWork *wk);
static void PokemonTrade_ClearDragIcon(PokemonTradeWork *wk);
static void PokemonTrade_ShowDragIcon(PokemonTradeWork *wk);
static void PokemonTrade_BeginDrag(PokemonTradeWork *wk, ClActor *icon, int a2, int a3, BoxPkm *pkm);
static void PokemonTrade_StepNone(PokemonTradeWork *wk);
static void *func_ov194_021b7bf0(int netId, void *work, int size);
static void *func_ov194_021b7c0c(int netId, void *work, int size);
static void *PokemonTrade_GetNetIdOffset(int netId, void *work, int size);
static void PokemonTrade_RecvPlayerMonA(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvMonCopy(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvMonChunkCommon(int netId, int size, const void *data, PokemonTradeWork *work,
                                            NetHandle *handle, int index);
static void func_ov194_021b7e0c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7e20(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvMonChunk2(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvOfferMonCommon(u32 value, int index, int netId, PokemonTradeWork *wk, NetHandle *handle);
static void PokemonTrade_RecvOfferMon0(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvOfferMon1(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvOfferMon2(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPlayerFlag(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerSelection(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7f1c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerFlags(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerState(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvTradeSummary(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvStateByte(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerByte(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerWord(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerWordB(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerHalf(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_RecvPeerByteB(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8134(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8140(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8164(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8170(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_WaitFadeOutToEnd(PokemonTradeWork *wk);
static void PokemonTrade_ClearSideMon(PokemonTradeWork *wk, int side);
static BOOL PokemonTrade_IsSelectedOfferSlot(PokemonTradeWork *wk, int box, int slot);
static BOOL PokemonTrade_IsBadPkm(PartyPkm *pkm);
static void PokemonTrade_BuildPartyPacket(PokemonTradeWork *wk);
static void PokemonTrade_ShowIntroMessage(PokemonTradeWork *wk);
static void PokemonTrade_PlaceCursorIcon(PokemonTradeWork *wk, int side, BOOL a2);
static void PokemonTrade_StepReturnFromSummary(PokemonTradeWork *wk);
static void PokemonTrade_StepMain(PokemonTradeWork *wk);
static void PokemonTrade_StepAfterSummaryWipe(PokemonTradeWork *wk);
static void PokemonTrade_StepMainMenuChoice(PokemonTradeWork *wk);
static void PokemonTrade_StepShowTradeSummary(PokemonTradeWork *wk);
static void PokemonTrade_StepOpenMainMenu(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitSync20(PokemonTradeWork *wk);
static void PokemonTrade_StepStartSync20(PokemonTradeWork *wk);
static void PokemonTrade_StepIntro(PokemonTradeWork *wk);
static void PokemonTrade_StepSendIntroReady(PokemonTradeWork *wk);
static void PokemonTrade_StepMenuChoice(PokemonTradeWork *wk);
static void PokemonTrade_StepOpenMenu(PokemonTradeWork *wk);
static void PokemonTrade_StepSendPartyBoxSelected(PokemonTradeWork *wk);
static void PokemonTrade_HideTouchbarButtons(PokemonTradeWork *wk);
static void PokemonTrade_SetDragTarget(PokemonTradeWork *wk, u32 a1, u32 a2);
static int func_ov194_021b8c84(PokemonTradeWork *wk, BOOL a1);
static void PokemonTrade_StepWaitKeyAfterMessage(PokemonTradeWork *wk);
static void func_ov194_021b8e2c(PokemonTradeWork *wk, u32 msg, BOOL a2);
static void func_ov194_021b8e74(PokemonTradeWork *wk);
static void func_ov194_021b8e80(PokemonTradeWork *wk);
static void func_ov194_021b8e8c(PokemonTradeWork *wk);
static void PokemonTrade_ShowPeerSummary(PokemonTradeWork *wk);
static void func_ov194_021b8ef4(PokemonTradeWork *wk);
static void func_ov194_021b8f1c(PokemonTradeWork *wk);
static void func_ov194_021b8f94(PokemonTradeWork *wk);
static void func_ov194_021b8fbc(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitTouchReleasePeer(PokemonTradeWork *wk);
static BOOL PokemonTrade_PollGridTouch(PokemonTradeWork *wk, int *box, int *slot, int *a3, int *a4);
static void PokemonTrade_StepPeerSummaryMenu(PokemonTradeWork *wk);
static void PokemonTrade_StepOpenPeerSummary(PokemonTradeWork *wk);
static void PokemonTrade_StepSendCursorChoice(PokemonTradeWork *wk);
static void PokemonTrade_StepResolveCursorResult(PokemonTradeWork *wk);
static void PokemonTrade_StepRebuildTradeScene(PokemonTradeWork *wk);
static void PokemonTrade_StepRestoreBrowseScene(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitThenRestoreScene(PokemonTradeWork *wk);
static void PokemonTrade_StepBeginWipeToRestore(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitSyncThenWipe(PokemonTradeWork *wk);
static void PokemonTrade_StepSendTradeData(PokemonTradeWork *wk);
static void PokemonTrade_StepBuildTradeRecord(PokemonTradeWork *wk);
static void PokemonTrade_StepBuildMarkingTable(PokemonTradeWork *wk);
static void PokemonTrade_StepSendBoxInfo(PokemonTradeWork *wk);
static void PokemonTrade_StepLoadBoxIconGfx(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitNetSync22(PokemonTradeWork *wk);
static void PokemonTrade_StepAfterWipeLoadBox(PokemonTradeWork *wk);
static void PokemonTrade_StepRestartNetSession(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitMessageThenResolve(PokemonTradeWork *wk);
static void PokemonTrade_StepClearOffersNewRound(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitMessageThenClear(PokemonTradeWork *wk);
static void PokemonTrade_StepSendContinue(PokemonTradeWork *wk);
static void PokemonTrade_StepSendStartRound(PokemonTradeWork *wk);
static void PokemonTrade_StepSummaryMenuChoice(PokemonTradeWork *wk);
static void PokemonTrade_StepOpenSummaryMenu(PokemonTradeWork *wk);
static void PokemonTrade_StepShowSummaryMessage(PokemonTradeWork *wk);
static void PokemonTrade_StepSlideInSubScreen(PokemonTradeWork *wk);
static BOOL PokemonTrade_FindMatchingBoxIcon(PokemonTradeWork *wk, u32 a1);
static void PokemonTrade_StepCloseSubScreenDone(PokemonTradeWork *wk);
static void PokemonTrade_StepExitSubScreen(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitCursorFade(PokemonTradeWork *wk);
static void PokemonTrade_StepSubScreenList(PokemonTradeWork *wk);
static void PokemonTrade_StepOpenSubScreen(PokemonTradeWork *wk);
static void PokemonTrade_StepBeginOpenSubScreen(PokemonTradeWork *wk);
static int PokemonTrade_GetColumnX(int column);
static BOOL PokemonTrade_IsSlotVisible(PokemonTradeWork *wk, int column);
static void PokemonTrade_StepSendNewRound(PokemonTradeWork *wk);
static BOOL PokemonTrade_SendSideMon(PokemonTradeWork *wk);
static void PokemonTrade_PollGridKeys(PokemonTradeWork *wk);
static void PokemonTrade_Scroll(PokemonTradeWork *wk, BOOL playSound, BOOL send);
static void PokemonTrade_DragScrollGrid(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitBrowseRelease(PokemonTradeWork *wk);
static void PokemonTrade_StepClearPeerSelection(PokemonTradeWork *wk);
static void PokemonTrade_StepBeginBrowse(PokemonTradeWork *wk);
static void PokemonTrade_StepResetGrid(PokemonTradeWork *wk);
static void PokemonTrade_StepEnterBrowsing(PokemonTradeWork *wk);
static void PokemonTrade_UpdateTouchbarButtons(PokemonTradeWork *wk);
static void PokemonTrade_StepConfirmSendOffer(PokemonTradeWork *wk);
static void PokemonTrade_StepSendOfferConfirm(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitIconFlight(PokemonTradeWork *wk);
static BOOL PokemonTrade_IsTradeTargetValid(PokemonTradeWork *wk, int box, int slot);
static void PokemonTrade_StepTouchbarAction(PokemonTradeWork *wk);
static void PokemonTrade_StepWaitKeyThenBrowse(PokemonTradeWork *wk);
static void PokemonTrade_InitMcssSystem(PokemonTradeWork *wk);
static void PokemonTrade_ExitMcssSystem(PokemonTradeWork *wk);
static void PokemonTrade_InitDisplay(PokemonTradeWork *wk);
static void PokemonTrade_ExitDisplay(PokemonTradeWork *wk);
static void PokemonTrade_LoadSceneResources(PokemonTradeWork *wk);
static void PokemonTrade_ClampScrollColumn(PokemonTradeWork *wk, BOOL a1);
static void PokemonTrade_VBlank(TCB *tcb, void *data);
static void PokemonTrade_SetupGameData(PokemonTradeWork *wk, GameData *gameData, u16 friendIndex);
static void PokemonTrade_FreeMons(PokemonTradeWork *wk, BOOL a1);
static void PokemonTrade_SetupBoxCounts(PokemonTradeWork *wk);
static void PokemonTrade_BuildTradeRecords(PokemonTradeWork *wk, int box);
static void PokemonTrade_RestoreBGM(PokemonTradeWork *wk);
static PokemonTradeWork *PokemonTrade_CreateWork(GameProc *proc, u32 heapSize);
static BOOL PokemonTrade_Init(GameProc *proc, u32 *state, PokemonTradeParam *param, PokemonTradeWork *wk, int type);
static BOOL PokemonTrade_ProcInitWifiClub(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcInitIrc(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_StartRequest1(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcInitGtsNego(GameProc *proc, u32 *state, void *param, void *work);
static void PokemonTrade_SetupMcssPokemon(PokemonTradeDemoParam *demo, PokemonTradeWork *wk);
static BOOL PokemonTrade_StartRequest4(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_StartRequest7(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_StartRequest8(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_StartRequest6(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_StepPeerSelectAction(PokemonTradeWork *wk);
static void PokemonTrade_StepBrowseGrid(PokemonTradeWork *wk);

// MWCC lays the file's data out by a heapsort over the order of declaration, which is not stable: the tables below
// are declared where they are, and the proc tables at the end in their order, because that gives the original's
// `.rodata` (see `tools/decomp/rodata_order.py`)
static const TouchRect sSideButtonRects[3];

static const NetCommand sNetCommands[] = {
    { PokemonTrade_RecvStateByte, NULL },
    { PokemonTrade_RecvMonCopy, func_ov194_021b7bf0 },
    { func_ov194_021b8170, NULL },
    { func_ov194_021b8140, NULL },
    { func_ov194_021b8134, NULL },
    { func_ov194_021b8164, NULL },
    { PokemonTrade_RecvPeerWordB, NULL },
    { func_ov194_021b7e0c, func_ov194_021b7c0c },
    { func_ov194_021b7e20, func_ov194_021b7c0c },
    { PokemonTrade_RecvMonChunk2, func_ov194_021b7c0c },
    { PokemonTrade_RecvPeerByte, NULL },
    { PokemonTrade_RecvPeerWord, NULL },
    { PokemonTrade_RecvPeerHalf, NULL },
    { PokemonTrade_RecvPeerByteB, NULL },
    { func_ov194_021b7f1c, PokemonTrade_GetNetIdOffset },
    { PokemonTrade_RecvPlayerMonA, NULL },
    { PokemonTrade_RecvPeerFlags, NULL },
    { PokemonTrade_RecvPeerState, NULL },
    { PokemonTrade_RecvTradeSummary, NULL },
    { PokemonTrade_RecvOfferMon0, NULL },
    { PokemonTrade_RecvOfferMon1, NULL },
    { PokemonTrade_RecvOfferMon2, NULL },
    { PokemonTrade_RecvPlayerFlag, NULL },
    { PokemonTrade_RecvPeerSelection, NULL },
};

// The memory of the trade's heap
static u8 sHeapMemory[0xc000];

void PokemonTrade_ResetTouchbarBackButton(PokemonTradeWork *wk) {
    TouchBar_Reset(wk->touchBar, TOUCHBAR_ICON_RETURN);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, TRUE);
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_RETURN, TRUE);
}

static BOOL PokemonTrade_IsMailBoxFull(PokemonTradeWork *wk) {
    MailData *save = MailBox_GetFromGameData(wk->gameData);
    BOOL ret = FALSE;
    if (MailBox_GetFreeSlot(save, 0) == -1) {
        ret = TRUE;
    }
    return ret;
}

void PokemonTrade_ClearNetAutoExit(PokemonTradeWork *wk) {
    GflNet_SetAutoExit(FALSE);
    GFL_NetSetDisconnectError(FALSE);
    NetState_WifiDisconnect(FALSE);
    wk->param->next = 3;
}

BOOL PokemonTrade_IsMonValid(const u8 *data) {
    int size = PokeParty_GetPkmRawSize();
    int i;
    if (data == NULL) {
        return FALSE;
    }
    for (i = 0; i < size; i++) {
        if (data[i] != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 PokemonTrade_GetStoredBoxIndex(PokemonTradeWork *wk) {
    return wk->partnerBoxCount;
}

BOOL PokemonTrade_IsNetwork(PokemonTradeWork *wk) {
    if (wk->param == NULL) {
        return FALSE;
    }
    if (wk->type >= 4) {
        return FALSE;
    }
    if (GFL_NetIsInitialized()) {
        return TRUE;
    }
    return FALSE;
}

static void PokemonTrade_UpdateBGM(PokemonTradeWork *wk) {
    if (wk->bgmTimer != 0) {
        wk->bgmTimer++;
        if (wk->bgmTimer == 2) {
            GFL_SndBGMFadeOut(6);
        } else if (wk->bgmTimer == 8) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            wk->bgmCount++;
            GFL_SndBGMPlay(SEQ_BGM_KOUKAN, 0xffff);
            GFL_SndBGMFadeIn(1);
            wk->bgmTimer = 0;
        }
    }
}

BOOL PokemonTrade_IsNegoType(PokemonTradeWork *wk) {
    BOOL ret = FALSE;
    switch (wk->type) {
    case 0:
    case 4:
    case 6:
    case 7:
    case 8:
        break;
    default:
        ret = TRUE;
        break;
    }
    return ret;
}

BOOL PokemonTrade_IsWifiMode(PokemonTradeWork *wk) {
    BOOL ret = FALSE;
    if (wk->type == 3 || wk->type == 9) {
        ret = TRUE;
    }
    return ret;
}

int PokemonTrade_GetColumnBox(int a, PokemonTradeWork *wk) {
    if (a >= 2) {
        return (a - 2) / 6;
    }
    return wk->boxCount;
}

int PokemonTrade_GetColumnSlot(int a, int b) {
    if (a >= 2) {
        return (a - 2) % 6 + b * 6;
    }
    if (b < 3) {
        return a + b * 2;
    }
    return -1;
}

static void PokemonTrade_RequestBoxRedraw(PokemonTradeWork *wk) {
    wk->firstColumn++;
    wk->unk1084 = 1;
    PokemonTrade_Scroll(wk, 0, 0);
}

static void PokemonTrade_UpdateDragState(PokemonTradeWork *wk) {
    u32 x, y;
    if (GFL_HIDGetTouchHeldPoint(&x, &y)) {
        if (wk->heldIcon == NULL) {
            wk->unkF5C = 0;
        }
        if (wk->heldIcon != NULL) {
            if (MATH_ABS(wk->heldPos.x - wk->heldOffsetX - (int)x) > 2) {
                wk->unkF5C = 1;
            }
            if (MATH_ABS(wk->heldPos.y - wk->heldOffsetY - (int)y) > 2) {
                wk->unkF5C = 1;
            }
            if (MATH_ABS(wk->heldPos.x - (int)x) > 6 && MATH_ABS(wk->heldPos.y - (int)y) > 6) {
                wk->unkF70 = 0;
            }
        }
        if (y >= 16 && y < 144) {
            wk->touchX = x;
            wk->touchY = y;
        }
    }
}

static void PokemonTrade_UpdateDragIcon(PokemonTradeWork *wk) {
    u32 x, y;
    ClActorPos pos;
    if (wk->heldIcon != NULL && GFL_HIDGetTouchHeldPoint(&x, &y)) {
        ClActor_GetSurfacePos(wk->heldIcon, &wk->heldPrevPos, 1);
        pos.x = x + wk->heldOffsetX;
        pos.y = y + wk->heldOffsetY;
        if (pos.y > 150) {
            pos.y = 150;
        }
        ClActor_SetSurfacePos(wk->heldIcon, &pos, 1);
    }
}

static void PokemonTrade_ClearDragIcon(PokemonTradeWork *wk) {
    if (wk->heldIcon != NULL) {
        wk->heldIcon = NULL;
    }
    Poketrade2D_DeleteFloatingIconActor(wk);
}

static void PokemonTrade_ShowDragIcon(PokemonTradeWork *wk) {
    if (wk->heldIcon != NULL) {
        ClActor_SetPriority(wk->heldIcon, 16);
        ClActor_SetSurfacePos(wk->heldIcon, &wk->heldPos, 1);
        ClActor_SetVisible(wk->heldIcon, TRUE);
        wk->heldSlot = -1;
        wk->heldBox = -1;
        PokemonTrade_ClearDragIcon(wk);
        MI_CpuFill8(wk->unk4, 0, sizeof(wk->unk4));
    }
}

static void PokemonTrade_BeginDrag(PokemonTradeWork *wk, ClActor *icon, int a2, int a3, BoxPkm *pkm) {
    u32 x, y;
    wk->heldIcon = icon;
    ClActor_GetSurfacePos(icon, &wk->heldPos, 1);
    Poketrade2D_SetIconObjMode(wk, wk->heldIcon, 1);
    if (GFL_HIDGetTouchHeldPoint(&x, &y)) {
        wk->heldOffsetX = wk->heldPos.x - x;
        wk->heldOffsetY = wk->heldPos.y - y;
        Poketrade2D_CreateFloatingIconActor(wk, a2, a3, x + wk->heldOffsetX, y + wk->heldOffsetY, pkm);
        wk->heldIcon = wk->actors[9];
    }
}

PartyPkm *PokemonTrade_CopyPartyPkm(BoxSaveAccessor *boxes, int box, int slot, PokemonTradeWork *wk) {
    PartyPkm *pkm = NULL;
    if (box != -1) {
        if (box > wk->boxCount) {
            return NULL;
        }
        if (box != wk->boxCount && slot != -1) {
            pkm = boxPkmRegenToPartyPkm(BoxSaveAccessor_GetPkm(boxes, box, slot), wk->heapId);
        } else {
            PokeParty *party = wk->party;
            if (slot < PokeParty_GetPkmCount(party) && slot != -1) {
                u32 size;
                pkm = PokeParty_NewTempPkm(1, 1, 1, wk->heapId);
                size = PokeParty_GetPkmRawSize();
                MI_CpuCopy8(PokeParty_GetPkm(party, slot), pkm, size);
            }
        }
    }
    return pkm;
}

BoxPkm *PokemonTrade_GetBoxPkm(BoxSaveAccessor *boxes, int box, int slot, PokemonTradeWork *wk) {
    PokeParty *party;
    if (box != -1) {
        if (box > wk->boxCount) {
            return NULL;
        }
        if (box != wk->boxCount && slot != -1) {
            return BoxSaveAccessor_GetPkm(boxes, box, slot);
        }
        party = wk->party;
        if (slot < PokeParty_GetPkmCount(party) && slot != -1) {
            return PokeParty_ToBoxPkm(PokeParty_GetPkm(party, slot));
        }
    }
    return NULL;
}

TradeBoxEntry *PokemonTrade_GetBoxEntry(int box, int slot, PokemonTradeWork *wk) {
    if (box != -1) {
        if (box > wk->boxCount) {
            return NULL;
        }
        if (box != wk->boxCount && slot != -1) {
            return &wk->boxEntries[box * 30 + 6 + slot];
        }
        if (slot >= 0 && slot < 6) {
            return &wk->boxEntries[slot];
        }
        return NULL;
    }
    return NULL;
}

static void PokemonTrade_StepNone(PokemonTradeWork *wk) {
}

void PokemonTrade_SetState(PokemonTradeWork *wk, PokemonTradeState state) {
    wk->state = state;
}

static void *func_ov194_021b7bf0(int netId, void *work, int size) {
    PokemonTradeWork *wk = work;
    if (netId >= 0 && netId < 2) {
        return wk->recvPkm[netId];
    }
    return NULL;
}

static void *func_ov194_021b7c0c(int netId, void *work, int size) {
    PokemonTradeWork *wk = work;
    if (netId >= 0 && netId < 2) {
        return wk->recvPkm[netId];
    }
    return NULL;
}

static void *PokemonTrade_GetNetIdOffset(int netId, void *work, int size) {
    PokemonTradeWork *wk = work;
    if (netId == PokemonTrade_GetMyNetId()) {
        return &wk->boxColors[0];
    }
    return &wk->boxColors[1];
}

static void PokemonTrade_RecvPlayerMonA(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn()) {
        wk->unk11E8[netId != PokemonTrade_GetMyNetId() ? 1 : 0] = *(const u8 *)data;
    }
}

u32 PokemonTrade_GetSideValue(PokemonTradeWork *wk, int side) {
    if (!PokemonTrade_IsNetwork(wk)) {
        return wk->unk5E4[side];
    }
    if (PokemonTrade_GetMyNetId() == 0) {
        return wk->unk5E4[side];
    }
    return wk->unk5E4[1 - side];
}

void PokemonTrade_SetSideValue(PokemonTradeWork *wk, int side, u32 value) {
    if (!PokemonTrade_IsNetwork(wk)) {
        wk->unk5E4[side] = value;
        return;
    }
    if (PokemonTrade_GetMyNetId() == 0) {
        wk->unk5E4[side] = value;
        return;
    }
    wk->unk5E4[1 - side] = value;
}

PartyPkm *PokemonTrade_GetPkm(PokemonTradeWork *wk, int side) {
    if (!PokemonTrade_IsNetwork(wk)) {
        return wk->pkm[side];
    }
    if (PokemonTrade_GetMyNetId() == 0) {
        return wk->pkm[side];
    }
    return wk->pkm[1 - side];
}

static void PokemonTrade_RecvMonCopy(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn()) {
        u32 pkmSize = PokeParty_GetPkmRawSize();
        MI_CpuCopy8(wk->recvPkm[netId], wk->pkm[netId], pkmSize);
        if (netId != PokemonTrade_GetMyNetId()) {
            wk->unkF9C = netId + 1;
            wk->unk11FB_4 = 1;
        }
    }
}

static void PokemonTrade_RecvMonChunkCommon(int netId, int size, const void *data, PokemonTradeWork *wk,
                                            NetHandle *handle, int index) {
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        if (PokeParty_GetParam(wk->recvPkm[netId], PKM_PARAM_SPECIES_VALID, NULL)) {
            PoketradeNego_StoreRemoteOffer(wk, index, wk->recvPkm[netId]);
            Poketrade2D_CreateOfferIconActor(wk, 1, index, wk->recvPkm[netId], 1, 0);
            PoketradeNego_ShowOfferSlotMon(wk, 1, wk->recvPkm[netId]);
        }
        wk->unk11FB_4 = 1;
    }
}

static void func_ov194_021b7e0c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTrade_RecvMonChunkCommon(netId, size, data, work, handle, 0);
}

static void func_ov194_021b7e20(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTrade_RecvMonChunkCommon(netId, size, data, work, handle, 1);
}

static void PokemonTrade_RecvMonChunk2(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTrade_RecvMonChunkCommon(netId, size, data, work, handle, 2);
}

static void PokemonTrade_RecvOfferMonCommon(u32 value, int index, int netId, PokemonTradeWork *wk, NetHandle *handle) {
    if (handle == NetHandle_GetOwn()) {
        if (netId == PokemonTrade_GetMyNetId()) {
            wk->unkFF0[0][index] = value;
        } else {
            wk->unkFF0[1][index] = value;
        }
    }
}

static void PokemonTrade_RecvOfferMon0(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTrade_RecvOfferMonCommon(*(const u8 *)data, 0, netId, work, handle);
}

static void PokemonTrade_RecvOfferMon1(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTrade_RecvOfferMonCommon(*(const u8 *)data, 1, netId, work, handle);
}

static void PokemonTrade_RecvOfferMon2(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTrade_RecvOfferMonCommon(*(const u8 *)data, 2, netId, work, handle);
}

static void PokemonTrade_RecvPlayerFlag(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn()) {
        wk->unk5E4[netId] = *(const u8 *)data;
    }
}

static void PokemonTrade_RecvPeerSelection(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        wk->unk11FB_0 = 0;
    }
}

static void func_ov194_021b7f1c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != NetHandle_GetOwn()) {
        return;
    }
    if (netId == PokemonTrade_GetMyNetId()) {
        return;
    }
}

static void PokemonTrade_RecvPeerFlags(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        const u8 *bytes = data;
        wk->partnerBoxCount = bytes[0];
        wk->unk11F4 = bytes[1];
        wk->unk107E = wk->partnerBoxCount * 160 + 16;
    }
}

static void PokemonTrade_RecvPeerState(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        wk->partnerCheckResult = *(const u8 *)data;
    }
}

static void PokemonTrade_RecvTradeSummary(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    UnityTowerVisitor profile;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(wk->gameData));
        u8 country, province, validCountry, validProvince;
        MI_CpuCopy8(data, &profile, sizeof(UnityTowerVisitor));
        country = UnityTowerVisitor_GetCountry(&profile.info);
        province = UnityTowerVisitor_GetProvince(&profile.info);
        validCountry = Country_GetValidCountry(country, province, TrainerInfo_GetRegion(&profile.info));
        validProvince = Country_GetValidRegion(country, province, TrainerInfo_GetRegion(&profile.info));
        if (country == validCountry && province == validProvince) {
            PlayerInfo_SetCountryAndProvince(&profile.info, country, province);
        } else {
            PlayerInfo_SetCountryAndProvince(&profile.info, 0, 0);
        }
        UnityTowerSurvey_RegisterTrade(survey, &profile);
    }
}

static void PokemonTrade_RecvStateByte(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn()) {
        wk->command[netId] = *(const u8 *)data;
    }
}

static void PokemonTrade_RecvPeerByte(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        wk->unk1060 = *(const u8 *)data;
    }
}

static void PokemonTrade_RecvPeerWord(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        PoketradeNego_RemoveOffer(wk, 1, *(const u32 *)data);
        Poketrade2D_DeleteOfferIconActor(wk, 1, *(const u32 *)data);
    }
}

static void PokemonTrade_RecvPeerWordB(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != PokemonTrade_GetMyNetId()) {
        PokemonTrade_ClearSideMon(wk, 1);
    }
}

static void PokemonTrade_RecvPeerHalf(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != NetSystem_GetNetId()) {
        wk->unk107E = *(const s16 *)data;
    }
}

static void PokemonTrade_RecvPeerByteB(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != NetSystem_GetNetId()) {
        PoketradeNego_FlashMarker(wk, *(const u8 *)data, 1);
    }
}

static void func_ov194_021b8134(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != NetHandle_GetOwn()) {
        return;
    }
}

static void func_ov194_021b8140(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == NetHandle_GetOwn() && netId != NetSystem_GetNetId()) {
        wk->unkFA0 = *(const u8 *)data;
    }
}

static void func_ov194_021b8164(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != NetHandle_GetOwn()) {
        return;
    }
}

static void func_ov194_021b8170(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != NetHandle_GetOwn()) {
        return;
    }
}

static void PokemonTrade_WaitFadeOutToEnd(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        PokemonTrade_SetState(wk, NULL);
    }
}

void PokemonTrade_FadeOutToEnd(PokemonTradeWork *wk) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, PokemonTrade_WaitFadeOutToEnd);
}

void PokemonTrade_SetSideMon(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    if (pkm != NULL) {
        Poketrade3D_DeleteMcssSlot(wk, side);
        Poketrade3D_AddPokemonMcssFlipped(wk, side, 1 - side, pkm, 1, PokemonTrade_IsNegoType(wk));
        PoketradeSave_DrawOfferCardWindow(wk, side, pkm);
    }
}

static void PokemonTrade_ClearSideMon(PokemonTradeWork *wk, int side) {
    int i;
    if (!PokemonTrade_IsNegoType(wk)) {
        Poketrade3D_DeleteMcssSlot(wk, side);
        PoketradeSave_DeleteOfferCardWindow(wk, side);
    } else {
        for (i = 0; i < 3; i++) {
            Poketrade2D_DeleteOfferIconActor(wk, side, i);
        }
    }
}

BOOL PokemonTrade_IsSelfMonReady(PokemonTradeWork *wk) {
    u32 value;
    PartyPkm *pkm;
    if (!PokemonTrade_IsNegoType(wk)) {
        pkm = PokemonTrade_GetPkm(wk, 0);
        value = PokemonTrade_GetSideValue(wk, 0);
    } else {
        pkm = PokemonTrade_GetPkm(wk, 1);
        value = PokemonTrade_GetSideValue(wk, 1);
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL) == 0) {
        return FALSE;
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        return FALSE;
    }
    if (countActivePkms(wk->party) == 1 && value != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PokemonTrade_IsSelectedOfferSlot(PokemonTradeWork *wk, int box, int slot) {
    BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, box, slot, wk);
    BOOL ret;
    if (wk->type != 0) {
        return FALSE;
    }
    if (box != wk->boxCount) {
        return FALSE;
    }
    ret = FALSE;
    if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        return ret;
    }
    if (!doesPkmHaveTmMove(pkm, 0)) {
        ret = TRUE;
    }
    return ret;
}

// Whether the Pokémon is a bad egg or not a valid species
static BOOL PokemonTrade_IsBadPkm(PartyPkm *pkm) {
    int species;
    BOOL ret = FALSE;
    if (PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL) == TRUE) {
        return TRUE;
    }
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    if (species > 686) {
        ret = TRUE;
    }
    return ret;
}

BOOL PokemonTrade_IsCurrentSideMonInvalid(PokemonTradeWork *wk) {
    int side = 0;
    if (PokemonTrade_IsNegoType(wk)) {
        side = 1;
    }
    return PokemonTrade_IsBadPkm(PokemonTrade_GetPkm(wk, side));
}

BOOL PokemonTrade_IsOtherSideMonInvalid(PokemonTradeWork *wk) {
    int side = 1;
    if (PokemonTrade_IsNegoType(wk)) {
        side = 0;
    }
    return PokemonTrade_IsBadPkm(PokemonTrade_GetPkm(wk, side));
}

BOOL PokemonTrade_IsMonTradeable(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    u32 value;
    if (!PokemonTrade_IsNegoType(wk)) {
        pkm = PokemonTrade_GetPkm(wk, 0);
        value = PokemonTrade_GetSideValue(wk, 1);
    } else {
        pkm = PokemonTrade_GetPkm(wk, 1);
        value = PokemonTrade_GetSideValue(wk, 0);
    }
    if (PML_ItemIsMail(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL)) && wk->unk11F4 != 0 && value == 0) {
        return TRUE;
    }
    return FALSE;
}

static void PokemonTrade_BuildPartyPacket(PokemonTradeWork *wk) {
    BOOL sent;
    BOOL network = PokemonTrade_IsNetwork(wk);
    if (!PoketradeSave_IsMessagePrintDone(wk)) {
        return;
    }
    if (network) {
        if (PokemonTrade_IsSelfMonReady(wk)) {
            sent = PokemonTrade_SendSelect(5);
        } else if (PokemonTrade_IsCurrentSideMonInvalid(wk)) {
            sent = PokemonTrade_SendSelect(7);
        } else if (PokemonTrade_IsOtherSideMonInvalid(wk)) {
            sent = PokemonTrade_SendSelect(8);
        } else if (PokemonTrade_IsMonTradeable(wk)) {
            sent = PokemonTrade_SendSelect(6);
        } else {
            sent = PokemonTrade_SendSelect(2);
        }
        if (sent) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1b, 8);
            PokemonTrade_SetState(wk, PokemonTrade_StepResolveCursorResult);
        }
    } else {
        wk->command[0] = 2;
        wk->command[1] = 2;
        PokemonTrade_SetState(wk, PokemonTrade_StepResolveCursorResult);
    }
}

static void PokemonTrade_ShowIntroMessage(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    PokemonTrade_SetState(wk, PokemonTrade_BuildPartyPacket);
}

static void PokemonTrade_PlaceCursorIcon(PokemonTradeWork *wk, int side, BOOL a2) {
    if (side == 0) {
        ClActorPos pos = { 96, 16 };
        ClActor_SetSurfacePos(wk->actors[4], &pos, 1);
    } else {
        ClActorPos pos = { 160, 16 };
        ClActor_SetSurfacePos(wk->actors[4], &pos, 1);
    }
    if (GCTX_HIDIsUsingTouch()) {
        Poketrade2D_PlaceTouchbarActors(wk, side, 0);
        ClActor_SetVisible(wk->actors[4], FALSE);
    } else {
        Poketrade2D_PlaceTouchbarActors(wk, side, 1);
        ClActor_SetVisible(wk->actors[4], TRUE);
    }
    if (a2) {
        PartyPkm *pkm = PokemonTrade_GetPkm(wk, wk->cursor);
        PoketradeSave_ShowSummaryMon(wk, pkm, wk->cursor, 1);
    }
}

static void PokemonTrade_StepReturnFromSummary(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        GFL_BGSysSetBGEnabled(3, FALSE);
        PoketradeSave_CloseSummaryPage(wk, 1);
        GFL_BGSysSetBGEnabled(4, FALSE);
        Poketrade2D_LoadBackgroundGfx(wk);
        Poketrade3D_AddUnitAndInit(wk, 0);
        GFL_BGSysQueueScrLoad(3);
        G2_BlendNone();
        PokemonTrade_SetSideMon(wk, 0, PokemonTrade_GetPkm(wk, 0));
        PokemonTrade_SetSideMon(wk, 1, PokemonTrade_GetPkm(wk, 1));
        PokemonTrade_RequestBoxRedraw(wk);
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, PokemonTrade_StepShowTradeSummary);
    }
}

static void PokemonTrade_StepMain(PokemonTradeWork *wk) {
    BOOL done = FALSE;
    if (wk->unk11F9 != 0 || !GFL_WipeIsFinished()) {
        return;
    }
    if (GCTX_HIDIsUsingTouch()) {
        if (PokemonTrade_GetPressedKeys()) {
            GCTX_HIDSetUsingTouch(FALSE);
            PokemonTrade_PlaceCursorIcon(wk, wk->cursor, FALSE);
            return;
        }
    }
    GFL_BGSysSetBGEnabled(3, TRUE);
    switch (GFL_HIDGetTouchPressedRect(sSideButtonRects)) {
    case 0:
        if (wk->cursor != 0 || !GCTX_HIDIsUsingTouch()) {
            wk->cursor = 0;
            GCTX_HIDSetUsingTouch(TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            PokemonTrade_PlaceCursorIcon(wk, wk->cursor, TRUE);
        }
        break;
    case 1:
        if (wk->cursor != 1 || !GCTX_HIDIsUsingTouch()) {
            wk->cursor = 1;
            GCTX_HIDSetUsingTouch(TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            PokemonTrade_PlaceCursorIcon(wk, wk->cursor, TRUE);
        }
        break;
    case 2:
        done = TRUE;
        break;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_KEY_LEFT || PokemonTrade_GetPressedKeys() == PAD_KEY_RIGHT) {
        wk->cursor = 1 - wk->cursor;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        GCTX_HIDSetUsingTouch(FALSE);
        PokemonTrade_PlaceCursorIcon(wk, wk->cursor, TRUE);
    }
    TouchBar_Main(wk->touchBar);
    if (TouchBar_GetDecided(wk->touchBar) == TOUCHBAR_ICON_RETURN) {
        done = TRUE;
    }
    if (done) {
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, PokemonTrade_StepReturnFromSummary);
    }
}

static void PokemonTrade_StepAfterSummaryWipe(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->cursor = 0;
        PoketradeSave_OpenSummaryScreen(wk, PokemonTrade_GetPkm(wk, 0));
        PokemonTrade_RequestBoxRedraw(wk);
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, PokemonTrade_StepMain);
    }
}

static void PokemonTrade_StepMainMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, PokemonTrade_ShowIntroMessage);
            break;
        case 1:
            GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
            PokemonTrade_SetState(wk, PokemonTrade_StepAfterSummaryWipe);
            break;
        case 2:
            PokemonTrade_SetState(wk, PokemonTrade_StepSendNewRound);
            break;
        }
    }
}

static void PokemonTrade_StepShowTradeSummary(PokemonTradeWork *wk) {
    int i;
    GFL_MsgDataLoadStrbuf(wk->msgData, 17, wk->strbufTemplate);
    for (i = 0; i < 2; i++) {
        loadPokemonNicknameToStrbuf(wk->wordSet, i, PokemonTrade_GetPkm(wk, i));
    }
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
    PoketradeSave_OpenMessageWindowTop(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepOpenMainMenu);
}

static void PokemonTrade_StepOpenMainMenu(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        u32 items[] = { 4, 3, 5 };
        PoketradeSave_OpenTaskMenu(wk, items, NELEMS(items));
        GFL_BGSysSetEnabledBGsA(0x1f);
        TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
        PokemonTrade_SetState(wk, PokemonTrade_StepMainMenuChoice);
    }
}

static void PokemonTrade_StepWaitSync20(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x14, 8)) {
            PoketradeSave_CloseMessageWindowClear(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
        }
    } else {
        PoketradeSave_CloseMessageWindowClear(wk);
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
    }
}

static void PokemonTrade_StepStartSync20(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk) && wk->timer > 30) {
        if (PokemonTrade_IsNetwork(wk)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x14, 8);
        }
        PokemonTrade_SetState(wk, PokemonTrade_StepWaitSync20);
    }
}

static void PokemonTrade_StepIntro(PokemonTradeWork *wk) {
    int msg;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    if (!PokemonTrade_IsNetwork(wk)) {
        myId = 0;
        otherId = 1;
    }
    if (PokemonTrade_IsNetwork(wk) && !NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x1a, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 1, 1);
    if (wk->command[myId] == 1 && wk->command[otherId] == 1) {
        PokemonTrade_SetState(wk, PokemonTrade_StepShowTradeSummary);
        return;
    }
    if (wk->command[myId] == 4 && wk->command[otherId] == 4) {
        wk->param->next = 3;
        GFL_MsgDataLoadStrbuf(wk->msgData, 142, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PoketradeSave_ShowTimeIcon(wk);
        wk->timer = 0;
        PokemonTrade_SetState(wk, PokemonTrade_StepStartSync20);
        return;
    }
    if (wk->command[otherId] == 4) {
        msg = 97;
    } else if (wk->command[myId] == 4) {
        msg = 103;
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    PokemonTrade_SetState(wk, PokemonTrade_StepShowWaitMessageA);
}

static void PokemonTrade_StepSendIntroReady(PokemonTradeWork *wk) {
    BOOL network = PokemonTrade_IsNetwork(wk);
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        if (!network) {
            wk->command[0] = 1;
            wk->command[1] = 1;
            PokemonTrade_SetState(wk, PokemonTrade_StepIntro);
        } else if (PokemonTrade_SendSelect(1)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1a, 8);
            PokemonTrade_SetState(wk, PokemonTrade_StepIntro);
        }
    }
}

static void PokemonTrade_StepMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        wk->menu = NULL;
        if (choice == 0) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
            PoketradeSave_OpenMessageWindowTop(wk);
            PoketradeSave_ShowTimeIcon(wk);
            PokemonTrade_SetState(wk, PokemonTrade_StepSendIntroReady);
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_StepResetGrid);
        }
    }
}

static void PokemonTrade_StepOpenMenu(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSideMon(wk)) {
        u32 items[] = { 0, 5 };
        PoketradeSave_OpenTaskMenu(wk, items, NELEMS(items));
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x1b, -8);
        PokemonTrade_SetState(wk, PokemonTrade_StepMenuChoice);
    }
}

static void PokemonTrade_StepSendPartyBoxSelected(PokemonTradeWork *wk) {
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK16, 1, &inParty)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepOpenMenu);
    }
}

static void PokemonTrade_HideTouchbarButtons(PokemonTradeWork *wk) {
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
    ClActor_SetVisible(wk->actors[2], FALSE);
    PokemonTrade_ClearDragIcon(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepSendPartyBoxSelected);
}

static void PokemonTrade_SetDragTarget(PokemonTradeWork *wk, u32 a1, u32 a2) {
    wk->cursorColumn = a1;
    wk->cursorRow = a2;
    PokemonTrade_RequestBoxRedraw(wk);
}

static BOOL PokemonTrade_PollGridTouch(PokemonTradeWork *wk, int *box, int *slot, int *a3, int *a4) {
    u32 x, y;
    int unused;
    if (GFL_HIDGetTouchPressedPoint(&x, &y) == TRUE) {
        if (Poketrade2D_HitTestBoxIcons(wk, x + 12, y + 12, box, slot, a3, a4, &unused)) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, *box, *slot, wk);
            if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static int func_ov194_021b8c84(PokemonTradeWork *wk, BOOL a1) {
    u32 x, y;
    int a, b, c;
    ClActor *icon;
    int ret = 0;
    if (GFL_HIDGetTouchPressedPoint(&x, &y) == TRUE) {
        icon = Poketrade2D_HitTestBoxIcons(wk, x + 12, y + 12, &wk->heldBox, &wk->heldSlot, &a, &b, &c);
        if (icon != NULL) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->heldBox, wk->heldSlot, wk);
            if (PokemonTrade_IsTradeTargetValid(wk, wk->heldBox, wk->heldSlot)) {
                GFL_SndSEPlay(SEQ_SE_MSCL_05);
                if (!PokemonTrade_IsNegoType(wk)) {
                    PokemonTrade_SetState(wk, PokemonTrade_StepClearPeerSelection);
                    return 3;
                }
                wk->unkF98 = wk->heldBox;
                wk->unkF94 = wk->heldSlot;
                PokemonTrade_SetState(wk, PoketradeNego_StepSendOfferRemoval);
                return 3;
            }
            PokemonTrade_ShowDragIcon(wk);
            if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                if (PokemonTrade_CanTradeMon(wk, pkm)) {
                    GFL_MsgDataLoadStrbuf(wk->msgData, 143, wk->strbuf);
                    PoketradeSave_OpenMessageWindowAtPos(wk, 1, 0);
                    PokemonTrade_SetState(wk, PokemonTrade_StepWaitKeyAfterMessage);
                    return 2;
                }
                PokemonTrade_BeginDrag(wk, icon, c, b, pkm);
                PokemonTrade_SetDragTarget(wk, a, b);
                if (a1) {
                    wk->unkF98 = wk->heldBox;
                    wk->unkF94 = wk->heldSlot;
                }
                PokemonTrade_RequestBoxRedraw(wk);
                ret = 1;
            }
        }
    }
    return ret;
}

static void PokemonTrade_StepWaitKeyAfterMessage(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk) && (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        wk->heldBox = -1;
        wk->heldSlot = -1;
        PoketradeSave_CloseMessageWindowClear(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
        wk->firstColumn = -1;
        Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 1);
    }
}

static void func_ov194_021b8e2c(PokemonTradeWork *wk, u32 msg, BOOL a2) {
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    PoketradeSave_OpenMessageWindowAtPos(wk, a2, 0);
    wk->firstColumn = -1;
    Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 0);
    PokemonTrade_SetState(wk, PokemonTrade_StepWaitKeyAfterMessage);
}

static void func_ov194_021b8e74(PokemonTradeWork *wk) {
    func_ov194_021b8e2c(wk, 101, FALSE);
}

static void func_ov194_021b8e80(PokemonTradeWork *wk) {
    func_ov194_021b8e2c(wk, 102, TRUE);
}

static void func_ov194_021b8e8c(PokemonTradeWork *wk) {
    func_ov194_021b8e2c(wk, 143, TRUE);
}

static void PokemonTrade_ShowPeerSummary(PokemonTradeWork *wk) {
    PartyPkm *pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
    PoketradeSave_CloseSummaryWindow(wk, 0);
    PoketradeSave_DrawSummaryPage(wk, wk->cursor, pkm);
    if (GCTX_HIDIsUsingTouch()) {
        PokemonTrade_RequestBoxRedraw(wk);
    }
    GFL_HeapFree(pkm);
    PokemonTrade_SetState(wk, PokemonTrade_StepPeerSummaryMenu);
}

static void func_ov194_021b8ef4(PokemonTradeWork *wk) {
    if (PoketradeNego_AddSelectedAsOffer(wk, wk->selectSlot, wk->selectBox)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepRebuildBrowseScene);
    }
}

static void func_ov194_021b8f1c(PokemonTradeWork *wk) {
    int index;
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    index = PoketradeNego_FindFreeOfferIndex(wk, 0, wk->selectSlot, wk->selectBox);
    if (index != -1) {
        if (GFL_NetSendCommand(NetHandle_GetOwn(), (u16)(TRADE_NET_CMD_UNK13 + index), 1, &inParty)) {
            PokemonTrade_SetState(wk, func_ov194_021b8ef4);
        }
    } else {
        PokemonTrade_SetState(wk, func_ov194_021b8ef4);
    }
}

static void func_ov194_021b8f94(PokemonTradeWork *wk) {
    if (PoketradeNego_AddSelectedAsOffer(wk, wk->selectSlot, wk->selectBox)) {
        PokemonTrade_SetState(wk, PoketradeNego_StepShowCancelMessage);
    }
}

static void func_ov194_021b8fbc(PokemonTradeWork *wk) {
    int index;
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    index = PoketradeNego_FindFreeOfferIndex(wk, 0, wk->selectSlot, wk->selectBox);
    if (index != -1) {
        if (GFL_NetSendCommand(NetHandle_GetOwn(), (u16)(TRADE_NET_CMD_UNK13 + index), 1, &inParty)) {
            PokemonTrade_SetState(wk, func_ov194_021b8f94);
        }
    } else {
        PokemonTrade_SetState(wk, func_ov194_021b8f94);
    }
}

static void PokemonTrade_StepWaitTouchReleasePeer(PokemonTradeWork *wk) {
    u32 x, y;
    if (!GFL_HIDGetTouchHeldPoint(&x, &y)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
    }
}

static void PokemonTrade_StepPeerSummaryMenu(PokemonTradeWork *wk) {
    u32 x, y;
    int a, b;
    BOOL selected = FALSE;
    BOOL showCursor = TRUE;
    int choice = -1;
    BOOL decided = FALSE;
    if (!AppTaskMenu_IsDecided(wk->menu)) {
        if (PokemonTrade_GetTouched()) {
            PokemonTrade_ShowDragIcon(wk);
        }
        TouchBar_Main(wk->touchBar);
        if (TouchBar_GetTouched(wk->touchBar) == TOUCHBAR_ICON_CUSTOM + 2 && GCTX_HIDIsUsingTouch()) {
            AppTaskMenu_SetCursorActive(wk->menu, FALSE);
        }
        if (TouchBar_GetDecided(wk->touchBar) == TOUCHBAR_ICON_CUSTOM + 2) {
            selected = TRUE;
            wk->cursor = 1 - wk->cursor;
        }
        if (GFL_HIDGetTouchPressedPoint(&x, &y) == TRUE) {
            GCTX_HIDSetUsingTouch(TRUE);
            AppTaskMenu_SetCursorActive(wk->menu, FALSE);
            if (PokemonTrade_PollGridTouch(wk, &wk->unkF98, &wk->unkF94, &a, &b)) {
                wk->cursorRow = b;
                wk->cursorColumn = a;
                wk->touchX = x;
                wk->touchY = y;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                selected = TRUE;
            } else if (y >= 16 && y < 144) {
                decided = TRUE;
            }
        }
        if (selected) {
            PoketradeSave_CloseSummaryWindow(wk, 1);
            PokemonTrade_SetState(wk, PokemonTrade_ShowPeerSummary);
            return;
        }
        PokemonTrade_UpdateDragIcon(wk);
        if (!decided && !GFL_HIDIsTouchHeld() && wk->touchHeld) {
            if (wk->heldIcon != NULL && wk->unkF5C) {
                GFL_SndSEPlay(SEQ_SE_SYS_56);
                choice = 0;
                decided = TRUE;
            } else {
                PokemonTrade_ShowDragIcon(wk);
            }
        }
        PokemonTrade_UpdateDragState(wk);
        PokemonTrade_UpdateDragIcon(wk);
    }
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        choice = AppTaskMenu_GetCursorPos(wk->menu);
        decided = TRUE;
    }
    if (decided) {
        if (choice == 0) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
            if (PokemonTrade_IsTradeTargetValid(wk, wk->unkF98, wk->unkF94) && PokemonTrade_IsNegoType(wk)) {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8f1c);
            } else if (hasPokemonChangedForm(pkm) == TRUE) {
                PokemonTrade_SetState(wk, func_ov194_021b8e80);
                PoketradeNego_SetOfferMarkersState(wk, 1);
            } else if (PokemonTrade_IsSelectedOfferSlot(wk, wk->unkF98, wk->unkF94)) {
                PoketradeNego_SetOfferMarkersState(wk, 1);
                PokemonTrade_SetState(wk, func_ov194_021b8e74);
            } else if (PokemonTrade_CanTradeMon(wk, pkm)) {
                PokemonTrade_SetState(wk, func_ov194_021b8e8c);
                PoketradeNego_SetOfferMarkersState(wk, 1);
            } else if (!PokemonTrade_IsNegoType(wk)) {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, PokemonTrade_HideTouchbarButtons);
                showCursor = FALSE;
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, FALSE);
            } else if (PoketradeNego_CanConfirmOffers(wk) && !GCTX_HIDIsUsingTouch()) {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8fbc);
                showCursor = FALSE;
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, FALSE);
            } else {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8f1c);
            }
        } else {
            PoketradeNego_SetOfferMarkersState(wk, 1);
            PokemonTrade_SetState(wk, PokemonTrade_StepWaitTouchReleasePeer);
        }
        wk->unkF98 = -1;
        wk->unkF94 = -1;
        if (showCursor) {
            PokemonTrade_UpdateTouchbarButtons(wk);
            ClActor_SetVisible(wk->actors[2], TRUE);
        }
        Poketrade2D_DeleteMarkingIcon(&wk->infoIcons[0]);
        Poketrade2D_DeleteMarkingIcon(&wk->infoIcons[1]);
        Poketrade2D_DeleteMarkingIcon(&wk->infoIcons[2]);
        PoketradeSave_CloseSummaryWindow(wk, 1);
        GFL_BGSysQueueScrLoad(6);
        Poketrade2D_ClearSummaryBackground(wk);
        PoketradeSave_CloseTaskMenu(wk);
        wk->menu = NULL;
    }
    wk->touchHeld = GFL_HIDIsTouchHeld();
}

// The two sides' buttons
static const TouchRect sSideButtonRects[] = {
    { 4, 28, 88, 112 },
    { 4, 28, 152, 176 },
    { TOUCH_RECT_END },
};

static void PokemonTrade_StepOpenPeerSummary(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    PoketradeNego_SetOfferMarkersState(wk, 0);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, TRUE);
    {
        u32 items[] = { 2, 5 };
        PoketradeSave_OpenTaskMenu(wk, items, NELEMS(items));
    }
    pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
    wk->cursor = 0;
    PoketradeSave_DrawSummaryPage(wk, 0, pkm);
    GFL_HeapFree(pkm);
    ClActor_SetVisible(wk->actors[2], FALSE);
    PokemonTrade_ShowDragIcon(wk);
    PokemonTrade_RequestBoxRedraw(wk);
    if (GCTX_HIDIsUsingTouch()) {
        AppTaskMenu_SetCursorActive(wk->menu, FALSE);
    } else {
        AppTaskMenu_SetCursorActive(wk->menu, TRUE);
    }
    PokemonTrade_SetState(wk, PokemonTrade_StepPeerSummaryMenu);
}

static void PokemonTrade_StepSendCursorChoice(PokemonTradeWork *wk) {
    u8 side = wk->cursor;
    if (!PokemonTrade_IsNetwork(wk) || GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK3, 1, &side)) {
        GXS_SetVisibleWnd(GX_WNDMASK_NONE);
        if (!PokemonTrade_IsNegoType(wk)) {
            PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Start);
        } else {
            PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Start);
        }
    }
}

void PokemonTrade_StepWaitCursorSync(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 13, 8)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepSendCursorChoice);
    }
}

static void PokemonTrade_StepResolveCursorResult(PokemonTradeWork *wk) {
    int msg;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    int mine, other;
    if (PokemonTrade_IsNetwork(wk) && !NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x1b, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 1, 1);
    if (wk->command[myId] == 2 && wk->command[otherId] == 2) {
        if (PokemonTrade_IsNetwork(wk)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 13, 8);
        }
        PokemonTrade_SetState(wk, PokemonTrade_StepWaitCursorSync);
        return;
    }
    mine = wk->command[myId];
    other = wk->command[otherId];
    if (other == 5) {
        msg = 97;
    } else if (mine == 5) {
        msg = 98;
    } else if (other == 7) {
        msg = 99;
    } else if (mine == 7) {
        msg = 100;
    } else if (other == 8) {
        msg = 100;
    } else if (mine == 8) {
        msg = 99;
    } else if (other == 9) {
        msg = 97;
    } else if (mine == 9) {
        msg = 101;
    } else if (other == 6) {
        msg = 97;
    } else if (mine == 6) {
        msg = 112;
    } else if (other == 3) {
        msg = 97;
    } else if (mine == 3) {
        msg = 103;
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    PokemonTrade_SetState(wk, PokemonTrade_StepShowWaitMessageA);
}

static void PokemonTrade_StepRebuildTradeScene(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNegoType(wk)) {
        PokemonTrade_StepResetGrid(wk);
        PokemonTrade_StepEnterBrowsing(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepWaitKeyThenBrowse);
    } else {
        PokemonTrade_StepResetGrid(wk);
        PokemonTrade_StepEnterBrowsing(wk);
    }
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
    GFL_BGSysSetEnabledBGsA(0x1f);
    GFL_BGSysSetEnabledBGsB(0x1e);
}

static void PokemonTrade_StepRestoreBrowseScene(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 18, 8)) {
        PoketradeSave_CloseMessageWindowClear(wk);
        if (PokemonTrade_IsNegoType(wk)) {
            PoketradeNego_ClearOfferSlots(wk);
            PoketradeNego_ExitMarkers(wk);
            if (wk->type == 2) {
                DWCRap_SetMic(FALSE);
            }
            GFL_MsgDataLoadStrbuf(wk->msgData, 135, wk->strbuf);
            PoketradeSave_OpenMessageWindowTop(wk);
            PoketradeNego_RegisterIconResources(wk, 0);
            PoketradeNego_DrawPlayerNames(wk);
            wk->unk11E8[0] = 0;
            wk->unk11E8[1] = 0;
            PoketradeNego_InitOfferMarkers(wk);
            PoketradeNego_SetOfferMarkersState(wk, 1);
            MI_CpuFill8(wk->iconSpecies, 0, sizeof(wk->iconSpecies));
            MI_CpuFill8(wk->iconForms, 0, sizeof(wk->iconForms));
            MI_CpuFill8(wk->iconSexes, 0, sizeof(wk->iconSexes));
            PokemonTrade_ShowDragIcon(wk);
            if (wk->menu != NULL) {
                AppTaskMenu_Free(wk->menu);
                wk->menu = NULL;
            }
            Poketrade2D_CreateBoxWindows(wk);
            Poketrade2D_SetupSubBlend(wk);
            PokemonTrade_UpdateTouchbarButtons(wk);
        }
        PokemonTrade_SetState(wk, PokemonTrade_StepRebuildTradeScene);
        PokemonTrade_Scroll(wk, 0, 1);
    }
}

static void PokemonTrade_StepWaitThenRestoreScene(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk) && GFL_WipeIsFinished()) {
        PoketradeSave_CloseMessageWindowClear(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepRestoreBrowseScene);
    }
}

static void PokemonTrade_StepBeginWipeToRestore(PokemonTradeWork *wk) {
    GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, PokemonTrade_StepWaitThenRestoreScene);
}

static void PokemonTrade_StepWaitSyncThenWipe(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 18, 8)) {
        Poketrade3D_UploadCubeTextures(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepBeginWipeToRestore);
    }
}

static void PokemonTrade_StepSendTradeData(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        if (GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, TRADE_NET_CMD_UNKE, sizeof(wk->boxColors[0]),
                                        &wk->boxColors[0], 0, FALSE, TRUE)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 18, 8);
            PokemonTrade_SetState(wk, PokemonTrade_StepWaitSyncThenWipe);
        }
    } else {
        Poketrade3D_UploadCubeTextures(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepWaitSyncThenWipe);
    }
}

static void PokemonTrade_StepBuildTradeRecord(PokemonTradeWork *wk) {
    PokemonTrade_BuildTradeRecords(wk, wk->timer - 1);
    if (wk->timer >= 24) {
        PokemonTrade_SetState(wk, PokemonTrade_StepSendTradeData);
    }
}

static void PokemonTrade_StepBuildMarkingTable(PokemonTradeWork *wk) {
    if (Poketrade2D_BuildMarkingTablesFrom(wk, wk->timer - 1)) {
        wk->timer = 0;
        PokemonTrade_SetState(wk, PokemonTrade_StepBuildTradeRecord);
    }
}

static void PokemonTrade_StepSendBoxInfo(PokemonTradeWork *wk) {
    u8 data[2];
    if (PokemonTrade_IsNetwork(wk)) {
        data[0] = wk->boxCount;
        data[1] = PokemonTrade_IsMailBoxFull(wk);
        if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK10, sizeof(data), data)) {
            wk->timer = 0;
            PokemonTrade_SetState(wk, PokemonTrade_StepBuildMarkingTable);
        }
    } else {
        wk->partnerBoxCount = wk->boxCount;
        wk->unk107E = wk->partnerBoxCount * 160 + 16;
        wk->timer = 0;
        PokemonTrade_SetState(wk, PokemonTrade_StepBuildMarkingTable);
    }
}

static void PokemonTrade_StepLoadBoxIconGfx(PokemonTradeWork *wk) {
    Poketrade2D_LoadBoxIconGfx(wk, wk->timer - 1);
    if (wk->timer > 25) {
        Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 0);
        PokemonTrade_SetState(wk, PokemonTrade_StepSendBoxInfo);
    }
}

static void PokemonTrade_StepWaitNetSync22(PokemonTradeWork *wk) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 22, 8)) {
        GflNet_SetAutoExit(TRUE);
        GFL_NetSetDisconnectError(TRUE);
        wk->timer = 0;
        PokemonTrade_SetState(wk, PokemonTrade_StepLoadBoxIconGfx);
    }
}

static void PokemonTrade_StepAfterWipeLoadBox(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        if (PokemonTrade_IsNetwork(wk)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 22, 8);
            PokemonTrade_SetState(wk, PokemonTrade_StepWaitNetSync22);
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_StepSendBoxInfo);
        }
    }
}

static void PokemonTrade_StepRestartNetSession(PokemonTradeWork *wk) {
    GFL_NetSetIconScreen(TRUE, wk->heapId);
    if (wk->type == 2) {
        DWCRap_SetMic(TRUE);
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
    PoketradeSave_OpenMessageWindowAtPos(wk, TRUE, 0);
    PoketradeSave_ShowTimeIcon(wk);
    GFL_BGSysSetEnabledBGsA(0x10);
    GFL_BGSysSetEnabledBGsB(4);
    killBrightnessEitherEngine(0);
    GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, PokemonTrade_StepAfterWipeLoadBox);
}

void PokemonTrade_StepResetForNewRound(PokemonTradeWork *wk) {
    int i;
    NetSystem_GetNetId();
    wk->selectBox = -1;
    wk->selectSlot = -1;
    wk->unk800 = 0;
    wk->heldIcon = NULL;
    wk->unk1060 = 0;
    Poketrade3D_DeleteMcssSlot(wk, 0);
    Poketrade3D_DeleteMcssSlot(wk, 1);
    if (PokemonTrade_IsNegoType(wk)) {
        PoketradeNego_ClearOfferSlots(wk);
    }
    MI_CpuFill8(wk->pkm[0], 0, PokeParty_GetPkmRawSize());
    MI_CpuFill8(wk->pkm[1], 0, PokeParty_GetPkmRawSize());
    for (i = 0; i < 10; i++) {
        if (wk->actors[i] != NULL) {
            ClActor_SetVisible(wk->actors[i], TRUE);
        }
    }
    PoketradeSave_CloseMessageWindowClear(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepRestartNetSession);
}

static void PokemonTrade_StepWaitMessageThenResolve(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1b, 8);
        PokemonTrade_SetState(wk, PokemonTrade_StepResolveCursorResult);
    }
}

static void PokemonTrade_StepSendNewRound(PokemonTradeWork *wk) {
    u8 command = 3;
    if (!PokemonTrade_IsNetwork(wk)) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 97, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepShowWaitMessageA);
        return;
    }
    if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_SELECT, 1, &command)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1b, 8);
        GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PoketradeSave_ShowTimeIcon(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepWaitMessageThenResolve);
    }
}

static void PokemonTrade_StepClearOffersNewRound(PokemonTradeWork *wk) {
    int i;
    if (!NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x19, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 1, 1);
    PoketradeSave_CloseMessageWindowClear(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    wk->unk11E8[0] = 0;
    wk->unk11E8[1] = 0;
    for (i = 0; i < 6; i++) {
        wk->negoSlot[i / 3][i % 3] = -1;
        if (wk->negoPkm[i / 3][i % 3] != NULL) {
            MI_CpuFill8(wk->negoPkm[i / 3][i % 3], 0, PokeParty_GetPkmRawSize());
        }
    }
    PokemonTrade_SetState(wk, PokemonTrade_StepResetGrid);
}

static void PokemonTrade_StepWaitMessageThenClear(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x19, 8);
        PokemonTrade_SetState(wk, PokemonTrade_StepClearOffersNewRound);
    }
}

void PokemonTrade_StepShowWaitMessageA(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk) && (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        PoketradeSave_CloseMessageWindowClear(wk);
        if (PokemonTrade_IsNetwork(wk)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 50, wk->strbuf);
            PoketradeSave_OpenMessageWindowTop(wk);
            PoketradeSave_ShowTimeIcon(wk);
            PokemonTrade_SetState(wk, PokemonTrade_StepWaitMessageThenClear);
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_StepResetGrid);
        }
    }
}

static void PokemonTrade_StepSendContinue(PokemonTradeWork *wk) {
    NetHandle *handle = NetHandle_GetOwn();
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            u8 command = 2;
            if (GFL_NetSendCommand(handle, TRADE_NET_CMD_UNKF, 1, &command)) {
                PokemonTrade_SetState(wk, PokemonTrade_StepIntro);
            }
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_StepIntro);
        }
    }
}

static void PokemonTrade_StepSendStartRound(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        if (PokemonTrade_SendSelect(4)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
            PoketradeSave_OpenMessageWindowTop(wk);
            PoketradeSave_ShowTimeIcon(wk);
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1a, 8);
            PokemonTrade_SetState(wk, PokemonTrade_StepSendContinue);
        }
    } else {
        wk->param->next = 3;
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
    }
}

static void PokemonTrade_StepSummaryMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, PokemonTrade_StepSendStartRound);
            break;
        case 1:
            PoketradeSave_CloseMessageWindowClear(wk);
            PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
            wk->firstColumn = -1;
            Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 0);
            Poketrade2D_SetupSubBlend(wk);
            break;
        }
    }
}

static BOOL PokemonTrade_SendSideMon(PokemonTradeWork *wk) {
    PartyPkm *pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->selectBox, wk->selectSlot, wk);
    if (PokemonTrade_IsNetwork(wk) &&
        !GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK1, PokeParty_GetPkmRawSize(), pkm)) {
        GFL_HeapFree(pkm);
        return FALSE;
    }
    PokemonTrade_SetSideMon(wk, 0, pkm);
    GFL_HeapFree(pkm);
    return TRUE;
}

// Where each box starts in the strip
static int sBoxStartX[] = {
    48,   208,  368,  528,  688,  848,  1008, 1168, 1328, 1488, 1648, 1808, 1968,
    2128, 2288, 2448, 2608, 2768, 2928, 3088, 3248, 3408, 3568, 3728, 3856,
};

// The boxes of the box list
static TouchRect sBoxListRects[] = {
    { 0x44, 0x5c, 0x08, 0x20 }, { 0x44, 0x5c, 0x20, 0x38 }, { 0x44, 0x5c, 0x38, 0x50 }, { 0x44, 0x5c, 0x50, 0x68 },
    { 0x44, 0x5c, 0x68, 0x80 }, { 0x44, 0x5c, 0x80, 0x98 }, { 0x44, 0x5c, 0x98, 0xb0 }, { 0x44, 0x5c, 0xb0, 0xc8 },
    { 0x44, 0x5c, 0xc8, 0xe0 }, { 0x44, 0x5c, 0xe0, 0xf8 }, { 0x5c, 0x74, 0x08, 0x20 }, { 0x5c, 0x74, 0x20, 0x38 },
    { 0x5c, 0x74, 0x38, 0x50 }, { 0x5c, 0x74, 0x50, 0x68 }, { 0x5c, 0x74, 0x68, 0x80 }, { 0x5c, 0x74, 0x80, 0x98 },
    { 0x5c, 0x74, 0x98, 0xb0 }, { 0x5c, 0x74, 0xb0, 0xc8 }, { 0x5c, 0x74, 0xc8, 0xe0 }, { 0x5c, 0x74, 0xe0, 0xf8 },
    { 0x74, 0x8c, 0x08, 0x20 }, { 0x74, 0x8c, 0x20, 0x38 }, { 0x74, 0x8c, 0x38, 0x50 }, { 0x74, 0x8c, 0x50, 0x68 },
    { 0x74, 0x8c, 0x68, 0x80 }, { 0x74, 0x8c, 0x80, 0x98 }, { TOUCH_RECT_END },
};

static void PokemonTrade_StepOpenSummaryMenu(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        u32 items[] = { 24, 25 };
        PoketradeSave_OpenTaskMenuAt(wk, items, NELEMS(items), 32, 12);
        wk->firstColumn = -1;
        Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 0);
        PokemonTrade_SetState(wk, PokemonTrade_StepSummaryMenuChoice);
    }
}

static void PokemonTrade_StepShowSummaryMessage(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 141, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    wk->unk1074 = 1;
    PokemonTrade_SetState(wk, PokemonTrade_StepOpenSummaryMenu);
}

static void PokemonTrade_StepSlideInSubScreen(PokemonTradeWork *wk) {
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x1b, -8);
    PokemonTrade_SetState(wk, PokemonTrade_StepShowSummaryMessage);
}

static BOOL PokemonTrade_FindMatchingBoxIcon(PokemonTradeWork *wk, u32 a1) {
    int slot;
    BoxPkm *pkm;
    int column = Poketrade2D_GetScrollColumn(wk);
    int i;
    for (i = 0; i < wk->columnCount; i++) {
        column++;
        if (column >= wk->columnCount) {
            column = 0;
        }
        for (slot = 0; slot < 5; slot++) {
            int box = PokemonTrade_GetColumnBox(column, wk);
            if ((pkm = PokemonTrade_GetBoxPkm(wk->boxes, box, PokemonTrade_GetColumnSlot(column, slot), wk)) != NULL &&
                Poketrade2D_IsBoxIconMatch(PML_PkmGetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL), a1) &&
                !PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
                wk->firstColumn++;
                wk->scrollX = PokemonTrade_GetColumnX(column) - 32;
                wk->unk1084 = 1;
                PokemonTrade_Scroll(wk, 0, 1);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void PokemonTrade_StepCloseSubScreenDone(PokemonTradeWork *wk) {
    GFL_BGSysSetEnabledBGsB(0x1e);
    PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
}

static void PokemonTrade_StepExitSubScreen(PokemonTradeWork *wk) {
    GFL_BGSysSetEnabledBGsB(0x1a);
    Poketrade2D_ClearSubScreen(wk);
    PoketradeSave_CloseMessageWindowClear(wk);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    PoketradeNego_SetOfferMarkersState(wk, 1);
    Poketrade2D_FadePaletteAll(wk, 0);
    PokemonTrade_UpdateTouchbarButtons(wk);
    Poketrade2D_SetupSubBlend(wk);
    Poketrade2D_DeleteCursorActor(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepCloseSubScreenDone);
}

static void PokemonTrade_StepWaitCursorFade(PokemonTradeWork *wk) {
    if (!Poketrade2D_ClearCursorIfDisabled(wk)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepExitSubScreen);
    }
}

static void PokemonTrade_StepSubScreenList(PokemonTradeWork *wk) {
    GFL_BGSysSetEnabledBGsB(0x1f);
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        int box = GFL_HIDGetTouchPressedRect(sBoxListRects);
        if (box != TOUCH_RECT_NONE) {
            wk->unk800 = box + 1;
            Poketrade2D_CreateCursorActor(wk, sBoxListRects[box].left, sBoxListRects[box].top);
            if (PokemonTrade_FindMatchingBoxIcon(wk, box)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                GXS_SetVisibleWnd(GX_WNDMASK_NONE);
                PokemonTrade_SetState(wk, PokemonTrade_StepWaitCursorFade);
                return;
            }
            GFL_MsgDataLoadStrbuf(wk->msgData, 52, wk->strbuf);
            PoketradeSave_OpenMessageWindow(wk, 1, 1, 1, 29, 4);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        TouchBar_Main(wk->touchBar);
        if (TouchBar_GetDecided(wk->touchBar) == TOUCHBAR_ICON_RETURN) {
            GFL_BGSysSetEnabledBGsB(0x1a);
            wk->unk800 = 0;
            Poketrade2D_ClearSubScreen(wk);
            PoketradeSave_CloseMessageWindowClear(wk);
            GXS_SetVisibleWnd(GX_WNDMASK_NONE);
            PoketradeNego_SetOfferMarkersState(wk, 1);
            Poketrade2D_FadePaletteAll(wk, 0);
            PokemonTrade_UpdateTouchbarButtons(wk);
            Poketrade2D_SetupSubBlend(wk);
            Poketrade2D_DeleteCursorActor(wk);
            PokemonTrade_SetState(wk, PokemonTrade_StepExitSubScreen);
        }
    }
}

static void PokemonTrade_StepOpenSubScreen(PokemonTradeWork *wk) {
    Poketrade2D_LoadSubScreenGfx(wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 51, wk->strbuf);
    PoketradeSave_OpenMessageWindow(wk, 1, 1, 1, 29, 4);
    PokemonTrade_SetState(wk, PokemonTrade_StepSubScreenList);
}

static void PokemonTrade_StepBeginOpenSubScreen(PokemonTradeWork *wk) {
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_CUSTOM, FALSE);
    PoketradeNego_SetOfferMarkersState(wk, 0);
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 10, -8);
    Poketrade2D_FadePaletteAll(wk, 1);
    if (PoketradeNego_HasOffer(wk)) {
        TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
    }
    PokemonTrade_SetState(wk, PokemonTrade_StepOpenSubScreen);
}

// The x of a column of the strip
static int PokemonTrade_GetColumnX(int column) {
    int n, box;
    if (column == 0) {
        return 0;
    }
    if (column == 1) {
        return 48;
    }
    n = column - 2;
    box = n / 6;
    n %= 6;
    return box * 160 + 96 + n * 26;
}

static BOOL PokemonTrade_IsSlotVisible(PokemonTradeWork *wk, int column) {
    int x = PokemonTrade_GetColumnX(column);
    s16 scroll = wk->scrollX;
    if (scroll < x && scroll + 224 > x) {
        return TRUE;
    }
    x += wk->stripWidth;
    if (scroll < x && scroll + 224 > x) {
        return TRUE;
    }
    return FALSE;
}

static void PokemonTrade_PollGridKeys(PokemonTradeWork *wk) {
    int i;
    int column;
    BOOL moved = FALSE;
    u32 keys = GCTX_HIDGetTypedKeys();
    if (wk->heldIcon != NULL) {
        return;
    }
    if (keys & PAD_BUTTON_R) {
        GCTX_HIDSetUsingTouch(FALSE);
        for (i = 0; i < wk->boxCount + 1; i++) {
            if (sBoxStartX[i] > wk->scrollX) {
                wk->scrollX = sBoxStartX[i];
                moved = TRUE;
                break;
            }
        }
        if (!moved) {
            moved = TRUE;
            wk->scrollX = sBoxStartX[0];
        }
        PokemonTrade_ClampScrollColumn(wk, moved);
    } else if (keys & PAD_BUTTON_L) {
        GCTX_HIDSetUsingTouch(FALSE);
        for (i = wk->boxCount; i >= 0; i--) {
            if (sBoxStartX[i] < wk->scrollX) {
                wk->scrollX = sBoxStartX[i];
                moved = TRUE;
                break;
            }
        }
        if (!moved) {
            moved = TRUE;
            wk->scrollX = sBoxStartX[wk->boxCount];
        }
        PokemonTrade_ClampScrollColumn(wk, moved);
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_UP) {
        GCTX_HIDSetUsingTouch(FALSE);
        if (!Poketrade2D_GetVisibleColumnRange(wk, &column)) {
            wk->cursorColumn = column;
        }
        wk->cursorRow--;
        if (wk->cursorRow < 0) {
            if (wk->cursorColumn < 2) {
                wk->cursorRow = 2;
            } else {
                wk->cursorRow = 4;
            }
        }
        wk->firstColumn--;
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_DOWN) {
        GCTX_HIDSetUsingTouch(FALSE);
        if (!Poketrade2D_GetVisibleColumnRange(wk, &column)) {
            wk->cursorColumn = column;
        }
        wk->cursorRow++;
        if (wk->cursorColumn < 2) {
            if (wk->cursorRow >= 3) {
                wk->cursorRow = 0;
            }
        } else if (wk->cursorRow >= 5) {
            wk->cursorRow = 0;
        }
        wk->firstColumn--;
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_RIGHT) {
        GCTX_HIDSetUsingTouch(FALSE);
        moved = TRUE;
        wk->firstColumn--;
        if (!Poketrade2D_GetVisibleColumnRange(wk, &column)) {
            wk->cursorColumn = column;
        } else {
            wk->cursorColumn++;
            if (!PokemonTrade_IsSlotVisible(wk, wk->cursorColumn)) {
                wk->scrollX = PokemonTrade_GetColumnX(wk->cursorColumn + 1) - 256;
                wk->firstColumn = -1;
            }
        }
        if (wk->cursorColumn >= wk->columnCount) {
            wk->cursorColumn = 0;
        }
        if (wk->cursorColumn < 2 && wk->cursorRow >= 3) {
            wk->cursorRow = 2;
        }
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_LEFT) {
        GCTX_HIDSetUsingTouch(FALSE);
        wk->firstColumn++;
        moved = TRUE;
        if (!Poketrade2D_GetVisibleColumnRange(wk, &column)) {
            wk->cursorColumn = column;
        } else {
            wk->cursorColumn--;
            if (wk->cursorColumn < 0) {
                wk->cursorColumn = wk->columnCount - 1;
            }
            if (!PokemonTrade_IsSlotVisible(wk, wk->cursorColumn)) {
                wk->scrollX = PokemonTrade_GetColumnX(wk->cursorColumn);
                wk->firstColumn = -1;
            }
        }
        if (wk->cursorColumn < 0) {
            wk->cursorColumn = wk->columnCount - 1;
        }
        if (wk->cursorColumn < 2 && wk->cursorRow >= 3) {
            wk->cursorRow = 2;
        }
    }
    if (moved) {
        wk->unk1084 = 1;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        PokemonTrade_Scroll(wk, 0, 1);
    }
}

static void PokemonTrade_Scroll(PokemonTradeWork *wk, BOOL playSound, BOOL send) {
    if (wk->scrollX < 0) {
        wk->scrollX += wk->stripWidth;
    }
    if (wk->stripWidth <= wk->scrollX) {
        wk->scrollX -= wk->stripWidth;
    }
    Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 1);
    Poketrade2D_DrawBoxGridScreen(wk);
    Poketrade2D_DrawBoxNames(wk);
    if (!GFL_SndPlayerIsActiveAny() && playSound) {
        GFL_SndSEPlay(SEQ_SE_SYS_57);
    }
    if (PokemonTrade_IsNetwork(wk) && send) {
        if (GFL_NetIsWifi()) {
            if (wk->scrollX % 16 == 0) {
                GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, TRADE_NET_CMD_UNKC, sizeof(wk->scrollX),
                                            &wk->scrollX, 0, TRUE, TRUE);
            }
        } else {
            GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, TRADE_NET_CMD_UNKC, sizeof(wk->scrollX), &wk->scrollX,
                                        0, TRUE, TRUE);
        }
    } else if (!PokemonTrade_IsNetwork(wk)) {
        wk->unk107E = wk->scrollX;
    }
}

static void PokemonTrade_DragScrollGrid(PokemonTradeWork *wk) {
    u32 x, y;
    u32 i;
    if (wk->heldIcon != NULL) {
        return;
    }
    if (GFL_HIDGetTouchHeldPoint(&x, &y)) {
        if (wk->unk1088) {
            if (x >= 64 && x < 192 && y >= 168 && y < 188) {
                ClActor_SetAutoAnim(wk->actors[2], TRUE);
                wk->scrollSpeed = (x - wk->scrollTouchX) * 2;
                wk->scrollTouchX = x;
                wk->scrollX -= wk->scrollSpeed;
                if (wk->scrollSpeed > 12) {
                    PokemonTrade_Scroll(wk, TRUE, TRUE);
                } else {
                    PokemonTrade_Scroll(wk, FALSE, TRUE);
                }
            } else {
                wk->scrollSpeed = 0;
            }
        }
        wk->scrollTouchX = x;
    } else {
        wk->scrollTouchX = 0;
    }
    if (!GFL_HIDIsTouchHeld() && wk->scrollSpeed != 0) {
        for (i = 2; i > 0 && wk->scrollSpeed != 0; i--) {
            if (wk->scrollSpeed < 0) {
                wk->scrollSpeed++;
            }
            if (wk->scrollSpeed > 0) {
                wk->scrollSpeed--;
            }
        }
        wk->scrollX -= wk->scrollSpeed;
        PokemonTrade_Scroll(wk, TRUE, TRUE);
    }
}

static void PokemonTrade_StepWaitBrowseRelease(PokemonTradeWork *wk) {
    if (!GFL_HIDIsTouchHeld()) {
        PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
    }
}

static void PokemonTrade_StepClearPeerSelection(PokemonTradeWork *wk) {
    PoketradeNego_FindOfferIndex(wk, wk->unkF98, wk->unkF94);
    if (!PokemonTrade_IsNetwork(wk) || GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK6, 0, NULL)) {
        TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
        wk->selectSlot = -1;
        wk->selectBox = -1;
        wk->heldSlot = -1;
        wk->heldBox = -1;
        wk->unkF94 = -1;
        wk->unkF98 = -1;
        PokemonTrade_ClearSideMon(wk, 0);
        PokemonTrade_RequestBoxRedraw(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepWaitBrowseRelease);
    }
}

static void PokemonTrade_StepBeginBrowse(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk)) {
        PokemonTrade_ClearSideMon(wk, 1);
        PokemonTrade_StepNone(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepEnterBrowsing);
    } else if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK6, 0, NULL)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepEnterBrowsing);
    }
}

static void PokemonTrade_StepResetGrid(PokemonTradeWork *wk) {
    wk->unk1088 = 0;
    wk->scrollTouchX = 0;
    wk->scrollSpeed = 0;
    MI_CpuFill8(wk->iconSpecies, 0, sizeof(wk->iconSpecies));
    MI_CpuFill8(wk->iconForms, 0, sizeof(wk->iconForms));
    MI_CpuFill8(wk->iconSexes, 0, sizeof(wk->iconSexes));
    PokemonTrade_ShowDragIcon(wk);
    PoketradeNego_SetOfferMarkersState(wk, 1);
    if (wk->menu != NULL) {
        AppTaskMenu_Free(wk->menu);
        wk->menu = NULL;
    }
    PokemonTrade_ClearSideMon(wk, 0);
    PokemonTrade_SetState(wk, PokemonTrade_StepBeginBrowse);
}

static void PokemonTrade_StepEnterBrowsing(PokemonTradeWork *wk) {
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, TRUE);
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_CUSTOM, TRUE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, FALSE);
    PokemonTrade_ResetTouchbarBackButton(wk);
    ClActor_SetVisible(wk->actors[2], TRUE);
    Poketrade2D_CreateBoxWindows(wk);
    Poketrade2D_SetupSubBlend(wk);
    wk->selectSlot = -1;
    wk->selectBox = -1;
    PokemonTrade_RequestBoxRedraw(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
}

static void PokemonTrade_UpdateTouchbarButtons(PokemonTradeWork *wk) {
    ClActor_SetVisible(wk->actors[2], TRUE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, PoketradeNego_HasOffer(wk));
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, PoketradeNego_HasOffer(wk));
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, TRUE);
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_CUSTOM, TRUE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, FALSE);
    PokemonTrade_ResetTouchbarBackButton(wk);
    PokemonTrade_RequestBoxRedraw(wk);
}

void PokemonTrade_StepRebuildBrowseScene(PokemonTradeWork *wk) {
    PoketradeNego_SetOfferMarkersState(wk, 1);
    MI_CpuFill8(wk->iconSpecies, 0, sizeof(wk->iconSpecies));
    MI_CpuFill8(wk->iconForms, 0, sizeof(wk->iconForms));
    MI_CpuFill8(wk->iconSexes, 0, sizeof(wk->iconSexes));
    PokemonTrade_ShowDragIcon(wk);
    if (wk->menu != NULL) {
        AppTaskMenu_Free(wk->menu);
        wk->menu = NULL;
    }
    Poketrade2D_CreateBoxWindows(wk);
    Poketrade2D_SetupSubBlend(wk);
    PokemonTrade_UpdateTouchbarButtons(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
}

static void PokemonTrade_StepConfirmSendOffer(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNegoType(wk)) {
        if (!PoketradeNego_AddSelectedAsOffer(wk, wk->selectSlot, wk->selectBox)) {
            return;
        }
    } else if (!PokemonTrade_SendSideMon(wk)) {
        return;
    }
    wk->unkF98 = -1;
    wk->unkF94 = -1;
    wk->heldSlot = -1;
    wk->heldBox = -1;
    PokemonTrade_RequestBoxRedraw(wk);
    PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
}

static void PokemonTrade_StepSendOfferConfirm(PokemonTradeWork *wk) {
    int index;
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    if (PokemonTrade_IsNegoType(wk)) {
        index = PoketradeNego_FindFreeOfferIndex(wk, 0, wk->selectSlot, wk->selectBox);
        if (index != -1) {
            if (GFL_NetSendCommand(NetHandle_GetOwn(), (u16)(TRADE_NET_CMD_UNK13 + index), 1, &inParty)) {
                PokemonTrade_SetState(wk, PokemonTrade_StepConfirmSendOffer);
            }
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_StepConfirmSendOffer);
        }
    } else if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK16, 1, &inParty)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepConfirmSendOffer);
    }
}

static void PokemonTrade_StepWaitIconFlight(PokemonTradeWork *wk) {
    if (Poketrade2D_UpdateIconFlight(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            PokemonTrade_SetState(wk, PokemonTrade_StepSendOfferConfirm);
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_StepConfirmSendOffer);
        }
    }
}

static BOOL PokemonTrade_IsTradeTargetValid(PokemonTradeWork *wk, int box, int slot) {
    if (!PokemonTrade_IsNegoType(wk)) {
        if (wk->selectSlot == slot && wk->selectBox == box) {
            return TRUE;
        }
    } else if (PoketradeNego_FindOfferIndex(wk, box, slot) != -1) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PokemonTrade_StepPeerSelectAction(PokemonTradeWork *wk) {
    BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
    if (PokemonTrade_IsTradeTargetValid(wk, wk->unkF98, wk->unkF94)) {
        GFL_SndSEPlay(SEQ_SE_MSCL_05);
        if (!PokemonTrade_IsNegoType(wk)) {
            PokemonTrade_SetState(wk, PokemonTrade_StepClearPeerSelection);
            return TRUE;
        }
        PokemonTrade_SetState(wk, PoketradeNego_StepSendOfferRemoval);
        return TRUE;
    }
    if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
        GFL_SndSEPlay(SEQ_SE_SELECT4);
        TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
        PoketradeSave_CloseTaskMenu(wk);
        PokemonTrade_SetState(wk, PokemonTrade_StepOpenPeerSummary);
        return TRUE;
    }
    PokemonTrade_ShowDragIcon(wk);
    wk->unkF98 = -1;
    wk->unkF94 = -1;
    return FALSE;
}

static void PokemonTrade_StepBrowseGrid(PokemonTradeWork *wk) {
    int column;
    wk->unk11EA = 1;
    if (GFL_HIDIsTouchHeld()) {
        GCTX_HIDSetUsingTouch(TRUE);
        if (GCTX_HIDGetHeldKeys()) {
            return;
        }
    }
    if (GCTX_HIDIsUsingTouch() && PokemonTrade_GetPressedKeys()) {
        GCTX_HIDSetUsingTouch(FALSE);
        if (!Poketrade2D_GetVisibleColumnRange(wk, &column)) {
            wk->cursorColumn = column;
            if (wk->cursorColumn < 2 && wk->cursorRow >= 3) {
                wk->cursorRow = 2;
            }
        } else if (!PokemonTrade_IsSlotVisible(wk, wk->cursorColumn)) {
            int first = Poketrade2D_GetScrollColumn(wk);
            int middle = first + 5;
            int target = wk->cursorColumn;
            if (first + 10 >= wk->columnCount && wk->cursorColumn < 20) {
                target += wk->columnCount + 1;
            }
            if (middle < target) {
                wk->scrollX = PokemonTrade_GetColumnX(wk->cursorColumn + 1) - 256;
            } else {
                wk->scrollX = PokemonTrade_GetColumnX(wk->cursorColumn);
            }
            wk->firstColumn = -1;
            wk->unk1084 = 1;
            PokemonTrade_Scroll(wk, FALSE, TRUE);
            wk->scrollSpeed = 0;
            return;
        }
        PokemonTrade_RequestBoxRedraw(wk);
        wk->scrollSpeed = 0;
        return;
    }
    if (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched()) {
        wk->scrollSpeed = 0;
    }
    if (PokemonTrade_GetTouched()) {
        PokemonTrade_ShowDragIcon(wk);
        wk->unkF98 = -1;
        wk->unkF94 = -1;
    }
    PokemonTrade_UpdateDragState(wk);
    PokemonTrade_UpdateDragIcon(wk);
    PokemonTrade_PollGridKeys(wk);
    PoketradeNego_PollMarkerTouch(wk);
    if (PokemonTrade_GetPressedKeys() == PAD_BUTTON_A) {
        BoxPkm *pkm;
        int box;
        GCTX_HIDSetUsingTouch(FALSE);
        if (!Poketrade2D_GetVisibleColumnRange(wk, &column)) {
            wk->cursorColumn = column;
            if (wk->cursorColumn < 2) {
                if (wk->cursorRow >= 3) {
                    wk->cursorRow = 0;
                }
            } else if (wk->cursorRow >= 5) {
                wk->cursorRow = 0;
            }
        }
        box = PokemonTrade_GetColumnBox(wk->cursorColumn, wk);
        pkm = PokemonTrade_GetBoxPkm(wk->boxes, box, PokemonTrade_GetColumnSlot(wk->cursorColumn, wk->cursorRow), wk);
        if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            if (PokemonTrade_CanTradeMon(wk, pkm)) {
                GFL_MsgDataLoadStrbuf(wk->msgData, 143, wk->strbuf);
                PoketradeSave_OpenMessageWindowAtPos(wk, 1, 0);
                PokemonTrade_SetState(wk, PokemonTrade_StepWaitKeyAfterMessage);
                return;
            }
            wk->unkF98 = PokemonTrade_GetColumnBox(wk->cursorColumn, wk);
            wk->unkF94 = PokemonTrade_GetColumnSlot(wk->cursorColumn, wk->cursorRow);
            GCTX_HIDSetUsingTouch(FALSE);
            if (PokemonTrade_IsNegoType(wk) && PoketradeNego_FindOfferIndex(wk, wk->unkF98, wk->unkF94) != -1) {
                GFL_SndSEPlay(SEQ_SE_MSCL_05);
                PokemonTrade_SetState(wk, PoketradeNego_StepSendOfferRemoval);
                return;
            }
            pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
            PokemonTrade_ShowDragIcon(wk);
            Poketrade2D_PlaceBoxIconActors(wk);
            PokemonTrade_StepPeerSelectAction(wk);
            return;
        }
    }
    switch (func_ov194_021b8c84(wk, FALSE)) {
    case 2:
        return;
    case 3:
        return;
    case 1:
        GCTX_HIDSetUsingTouch(TRUE);
        wk->unkF70 = 1;
        break;
    }
    if (!GFL_HIDIsTouchHeld()) {
        if (wk->unk1088 && wk->unkF5C && wk->heldIcon != NULL) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->heldBox, wk->heldSlot, wk);
            wk->unk1088 = 0;
            wk->unkF5C = 0;
            if (hasPokemonChangedForm(pkm) == TRUE) {
                wk->heldBox = -1;
                wk->heldSlot = -1;
                PokemonTrade_ClearDragIcon(wk);
                PokemonTrade_SetState(wk, func_ov194_021b8e80);
                return;
            }
            if (PokemonTrade_IsSelectedOfferSlot(wk, wk->heldBox, wk->heldSlot)) {
                wk->heldBox = -1;
                wk->heldSlot = -1;
                PokemonTrade_ClearDragIcon(wk);
                PokemonTrade_SetState(wk, func_ov194_021b8e74);
                return;
            }
            wk->unkF98 = wk->heldBox;
            wk->unkF94 = wk->heldSlot;
            wk->selectSlot = wk->heldSlot;
            wk->selectBox = wk->heldBox;
            wk->heldIcon = NULL;
            TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, TRUE);
            TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, TRUE);
            Poketrade2D_StartIconFlight(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_56);
            PokemonTrade_SetState(wk, PokemonTrade_StepWaitIconFlight);
            return;
        }
        if (wk->unk1088 && wk->heldIcon != NULL && wk->unkF70) {
            wk->unkF98 = wk->heldBox;
            wk->unkF94 = wk->heldSlot;
            PokemonTrade_StepPeerSelectAction(wk);
            return;
        }
        if (wk->heldSlot != -1 && wk->heldBox != -1) {
            PokemonTrade_ShowDragIcon(wk);
            PokemonTrade_RequestBoxRedraw(wk);
        } else {
            PokemonTrade_ShowDragIcon(wk);
        }
        wk->unk1088 = 0;
        wk->unkF5C = 0;
        ClActor_SetAutoAnim(wk->actors[2], FALSE);
    }
    PokemonTrade_DragScrollGrid(wk);
    if (GFL_HIDIsTouchHeld()) {
        wk->unk1088 = 1;
    }
    TouchBar_Main(wk->touchBar);
    {
        u32 button = TouchBar_GetTouched(wk->touchBar);
        switch (button) {
        case TOUCHBAR_ICON_RETURN:
        case TOUCHBAR_ICON_CUSTOM:
        case TOUCHBAR_ICON_CUSTOM + 1:
            PokemonTrade_SetState(wk, PokemonTrade_StepTouchbarAction);
            break;
        }
    }
}

static void PokemonTrade_StepTouchbarAction(PokemonTradeWork *wk) {
    TouchBar_Main(wk->touchBar);
    switch (TouchBar_GetDecided(wk->touchBar)) {
    case TOUCHBAR_ICON_RETURN:
        PokemonTrade_SetState(wk, PokemonTrade_StepSlideInSubScreen);
        break;
    case TOUCHBAR_ICON_CUSTOM:
        PokemonTrade_SetState(wk, PokemonTrade_StepBeginOpenSubScreen);
        break;
    case TOUCHBAR_ICON_CUSTOM + 1:
        if (PokemonTrade_IsNegoType(wk)) {
            if (PoketradeNego_HasOffer(wk)) {
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
                PokemonTrade_ClearDragIcon(wk);
                PokemonTrade_SetState(wk, PoketradeNego_StepShowCancelMessage);
                return;
            }
            TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, TRUE);
            TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
        } else if (wk->selectSlot == -1) {
            TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM, TRUE);
            TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
        } else {
            PokemonTrade_SetState(wk, PokemonTrade_HideTouchbarButtons);
            return;
        }
        PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
        break;
    }
}

static void PokemonTrade_StepWaitKeyThenBrowse(PokemonTradeWork *wk) {
    BOOL done = FALSE;
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        if (PokemonTrade_GetPressedKeys()) {
            GCTX_HIDSetUsingTouch(FALSE);
            done = TRUE;
        }
        if (PokemonTrade_GetTouched()) {
            GCTX_HIDSetUsingTouch(TRUE);
            done = TRUE;
        }
        if (done) {
            PoketradeSave_CloseMessageWindowClear(wk);
            Poketrade2D_SetupSubBlend(wk);
            PokemonTrade_SetState(wk, PokemonTrade_StepBrowseGrid);
            wk->firstColumn = -1;
            Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 1);
        }
    }
}

static void PokemonTrade_InitMcssSystem(PokemonTradeWork *wk) {
    Poketrade2D_InitG3DSystem(wk);
    wk->mcssSys = MCSSSys_Create(4, HEAPID_TAIL(wk->heapId));
    MCSSSys_SetTexBase(wk->mcssSys, 0x30000);
    MCSSSys_EnableOrtho(wk->mcssSys);
}

static void PokemonTrade_ExitMcssSystem(PokemonTradeWork *wk) {
    Poketrade3D_DeleteMcssSlot(wk, 0);
    Poketrade3D_DeleteMcssSlot(wk, 1);
    MCSSSys_Free(wk->mcssSys);
    wk->mcssSys = NULL;
    GFL_G3DCameraFree(wk->camera);
    GFL_G3DSysFree();
}

static void PokemonTrade_InitDisplay(PokemonTradeWork *wk) {
    GFL_BGSysCreate(HEAPID_POKEMON_TRADE);
    BmpWin_InitAllocator(wk->heapId);
    Poketrade2D_InitDisplayAndClact(wk);
    Poketrade2D_SetupMainBgs(wk);
    Poketrade2D_SetupSubBgs(wk);
}

static void PokemonTrade_ExitDisplay(PokemonTradeWork *wk) {
    ClActSys_Delete();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void PokemonTrade_LoadSceneResources(PokemonTradeWork *wk) {
    Poketrade2D_RegisterIconCharResources(wk);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    PoketradeNego_ExitMarkers(wk);
    if (wk->type < 4) {
        Poketrade2D_RegisterIconSpriteResources(wk);
    }
    Poketrade2D_CreateBoxIconActors(wk);
    Poketrade2D_LoadMainGfxAndMessageWindow(wk);
    Poketrade2D_LoadBackgroundGfx(wk);
    Poketrade2D_CreateBoxWindows(wk);
    Poketrade2D_UpdateVisibleColumns(wk->boxes, wk, 0);
}

static void PokemonTrade_VBlank(TCB *tcb, void *data) {
    PokemonTradeWork *wk = data;
    ClActSys_VBlank();
    if (wk->unk1084) {
        GFL_BGSysMoveBG(7, BG_MOVE_SET_X, wk->unk108C);
        GFL_BGSysMoveBG(5, BG_MOVE_SET_X, wk->unk108C);
        wk->unk1084 = 0;
    }
}

static void PokemonTrade_SetupGameData(PokemonTradeWork *wk, GameData *gameData, u16 friendIndex) {
    u32 i;
    for (i = 0; i < 3; i++) {
        wk->recvPkm[i] =
            GFL_HeapAllocate(HEAPID_POKEMON_TRADE, PokeParty_GetPkmRawSize(), TRUE, "pokemontrade_proc.c", 4078);
    }
    if (gameData != NULL) {
        wk->gameData = gameData;
        wk->boxes = GameData_GetBoxSaveAccessor(gameData);
        wk->myInfo = GetGameDataPlayerInfo(gameData);
        wk->party = GameData_GetParty(gameData);
        if (wk->partnerInfo == NULL) {
            if (PokemonTrade_IsNetwork(wk)) {
                wk->partnerInfo = GameData_GetPlayerInfoByNetID(gameData, 1 - NetSystem_GetNetId());
            } else {
                wk->partnerInfo = GameData_GetPlayerInfoByNetID(gameData, 1);
            }
        }
    }
}

static void PokemonTrade_FreeMons(PokemonTradeWork *wk, BOOL a1) {
    u32 i;
    for (i = 0; i < 3; i++) {
        GFL_HeapFree(wk->recvPkm[i]);
    }
}

static void PokemonTrade_SetupBoxCounts(PokemonTradeWork *wk) {
    int count = BoxSaveAccessor_GetAvailableBoxCount(wk->boxes);
    wk->columnCount = count * 6 + 2;
    wk->boxCount = count;
    wk->stripTileWidth = count * 20 + 12;
    wk->stripWidth = count * 160 + 96;
}

// Fills the icons of the party (with the first box) or a box
static void PokemonTrade_BuildTradeRecords(PokemonTradeWork *wk, int box) {
    int i;
    BoxPkm *pkm;
    TradeBoxEntry *entries = wk->boxEntries;
    if (box == 0) {
        MI_CpuFill8(entries, 0, sizeof(wk->boxEntries));
        for (i = 0; i < 6; i++) {
            pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->boxCount, i, wk);
            if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                u32 species = PML_PkmGetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
                if (species != 0) {
                    TradeBoxEntry *entry = &entries[i];
                    entry->species = species;
                    entry->form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
                    entry->sex = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL);
                }
            }
        }
    }
    // One box a frame: the loop stops after its first pass, as the original's does
    while (box < 24) {
        for (i = 0; i < 30; i++) {
            int index = box * 30 + 6 + i;
            pkm = PokemonTrade_GetBoxPkm(wk->boxes, box, i, wk);
            if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                u32 species = PML_PkmGetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
                if (species != 0) {
                    TradeBoxEntry *entry = &entries[index];
                    entry->species = species;
                    entry->form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
                    entry->sex = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL);
                }
            }
        }
        break;
    }
}

void PokemonTrade_DeleteAllResources(PokemonTradeWork *wk) {
    int i;
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    Poketrade2D_DeleteAllOfferIconActors(wk);
    Poketrade3D_RemoveCurrentUnit(wk);
    PoketradeNego_ReleaseIconResources(wk);
    PoketradeNego_ResetMarkers(wk);
    PoketradeNego_DeleteHeaderWindows(wk);
    Poketrade2D_DeleteBoxWindows(wk);
    Poketrade3D_DeleteMcssSlot(wk, 1);
    Poketrade2D_DeletePickedIconActor(wk);
    PoketradeNego_UpdateOfferMarkers(wk);
    PoketradeSave_DeleteOfferListWindows(wk);
    Poketrade2D_DeleteAllBoxIcons(wk);
    PoketradeSave_DeleteOfferCardWindow(wk, 0);
    PoketradeSave_DeleteOfferCardWindow(wk, 1);
    if (wk->touchBar != NULL) {
        TouchBar_Free(wk->touchBar);
        wk->touchBar = NULL;
    }
    Poketrade2D_DeleteOfferIcons(wk);
    if (wk->clactUnit != NULL) {
        ClActUnit_Delete(wk->clactUnit);
        wk->clactUnit = NULL;
    }
    PoketradeSave_DeleteMessageResources(wk);
    PoketradeNego_FreeOfferMons(wk);
    PoketradeSave_DeletePartnerOfferCard(wk, 0);
    PoketradeSave_DeletePartnerOfferCard(wk, 1);
    Poketrade2D_FreeBackgroundPalette(wk);
    Poketrade2D_FreeMainResources(wk);
    Poketrade2D_ExitSubBgs(wk);
    if (wk->fade != NULL) {
        GFL_HeapFree(wk->fade);
        wk->fade = NULL;
    }
    for (i = 0; i < 4; i++) {
        if (wk->mcssMoves[i] != NULL) {
            GFL_HeapFree(wk->mcssMoves[i]);
            wk->mcssMoves[i] = NULL;
        }
    }
}

// Restores the music from before the trade
static void PokemonTrade_RestoreBGM(PokemonTradeWork *wk) {
    switch (wk->bgmCount) {
    case 0:
        break;
    case 2:
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        // fallthrough
    case 1:
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        GFL_SndBGMFadeIn(30);
        break;
    }
    wk->bgmCount = 0;
}

static PokemonTradeWork *PokemonTrade_CreateWork(GameProc *proc, u32 heapSize) {
    PokemonTradeWork *wk;
    GFL_HeapCreateRoot(sHeapMemory, sizeof(sHeapMemory), HEAPID_POKEMON_TRADE);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_IRC_BATTLE_MENU, heapSize);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokemonTradeWork), HEAPID_POKEMON_TRADE);
    MI_CpuFill8(wk, 0, sizeof(PokemonTradeWork));
    return wk;
}

static BOOL PokemonTrade_Init(GameProc *proc, u32 *state, PokemonTradeParam *param, PokemonTradeWork *wk, int type) {
    if (type < 5) {
        GFL_OvlLoad(OVERLAY_APP_UI);
        GFL_OvlLoad(OVERLAY_ID(189));
    }
    if (type == 0 || type == 4) {
        GFL_OvlLoad(OVERLAY_ID(192));
    } else {
        GFL_OvlLoad(OVERLAY_ID(193));
    }
    GFL_BGSysSetDisplayLayout(1);
    wk->heapId = HEAPID_IRC_BATTLE_MENU;
    wk->type = type;
    wk->param = param;
    wk->pkm[0] = GFL_HeapAllocate(HEAPID_POKEMON_TRADE, PokeParty_GetPkmRawSize(), TRUE, "pokemontrade_proc.c", 4379);
    wk->pkm[1] = GFL_HeapAllocate(HEAPID_POKEMON_TRADE, PokeParty_GetPkmRawSize(), TRUE, "pokemontrade_proc.c", 4380);
    wk->savedObjPalette = GFL_HeapAllocate(HEAPID_POKEMON_TRADE, 0x200, FALSE, "pokemontrade_proc.c", 4381);
    wk->savedBGPalette = GFL_HeapAllocate(HEAPID_POKEMON_TRADE, 0x200, FALSE, "pokemontrade_proc.c", 4382);
    wk->nationalDex = PokeDex_IsNationalObtained(GameData_GetPokedex(param->gameData));
    if (param != NULL) {
        PokemonTrade_SetupGameData(wk, param->gameData, param->unk2E);
    }
    PokemonTrade_SetupBoxCounts(wk);
    PokemonTrade_InitMcssSystem(wk);
    PokemonTrade_InitDisplay(wk);
    wk->clactUnit = ClActUnit_Create(340, 0, wk->heapId);
    if (PokemonTrade_IsNetwork(wk)) {
        NetCommand_AddTable(TRADE_NET_CMD_BASE, sNetCommands, NELEMS(sNetCommands), wk);
        GflNet_SetAutoExit(TRUE);
        GFL_NetSetDisconnectError(TRUE);
    }
    wk->vblankTcb = GFL_VBlankTCBAdd(PokemonTrade_VBlank, wk, 0);
    wk->heldIcon = NULL;
    wk->selectSlot = -1;
    wk->sceneId = -1;
    wk->heldSlot = -1;
    wk->heldBox = -1;
    wk->unkF98 = -1;
    wk->unkF94 = -1;
    Poketrade3D_Init3DScene(wk);
    if (param->next == 1) {
        GFL_BGSysSetEnabledBGsA(0x10);
        PokemonTrade_SetState(wk, PoketradeSave_StepSaveCancelNotice);
        param->next = 0;
        return TRUE;
    }
    PoketradeSave_CreateMessageResources(wk);
    Poketrade2D_CreateFontResource(wk);
    if (wk->type < 4) {
        PokemonTrade_LoadSceneResources(wk);
        PokemonTrade_CreateTouchBar(wk);
        GFL_NetSetIconScreen(TRUE, wk->heapId);
    }
    Poketrade3D_AddUnitAndInit(wk, 0);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    PokemonTrade_SetState(wk, PokemonTrade_StepRestartNetSession);
    return TRUE;
}

static BOOL PokemonTrade_ProcInitWifiClub(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    return PokemonTrade_Init(proc, state, param, wk, 2);
}

static BOOL PokemonTrade_ProcInitIrc(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    return PokemonTrade_Init(proc, state, param, wk, 0);
}

static BOOL PokemonTrade_StartRequest1(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeParam *trade = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    int i;
    for (i = 0; i < 2; i++) {
        wk->unk1048[i] = 0;
        wk->unk1050[i] = 1;
    }
    wk->partnerInfo = GameData_GetPlayerInfoByNetID(trade->gameData, 1 - NetSystem_GetNetId());
    return PokemonTrade_Init(proc, state, trade, wk, 1);
}

static BOOL PokemonTrade_ProcInitGtsNego(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeParam *trade = param;
    GtsNegoParam *nego = trade->nego;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    int i;
    if (nego != NULL) {
        if (nego->result == 2) {
            for (i = 0; i < 2; i++) {
                wk->unk1048[i] = nego->unk4[i].unk4;
                wk->unk1050[i] = nego->unk4[i].unk0;
            }
        } else {
            for (i = 0; i < 2; i++) {
                wk->unk1048[i] = -1;
                wk->unk1050[i] = -1;
            }
        }
    }
    return PokemonTrade_Init(proc, state, trade, wk, 3);
}

static void PokemonTrade_SetupMcssPokemon(PokemonTradeDemoParam *demo, PokemonTradeWork *wk) {
    MI_CpuCopy8(demo->pkm[0], wk->pkm[0], PokeParty_GetPkmRawSize());
    MI_CpuCopy8(demo->pkm[1], wk->pkm[1], PokeParty_GetPkmRawSize());
    Poketrade3D_AddPokemonMcssPlain(wk, 0, 1, demo->pkm[0], 1);
    Poketrade3D_AddPokemonMcssPlain(wk, 1, 1, demo->pkm[1], 1);
    MCSS_Hide(!PokemonTrade_IsNegoType(wk) ? wk->mcss[1] : wk->mcss[0]);
    wk->myInfo = demo->myInfo;
    wk->partnerInfo = demo->partnerInfo;
    GFL_NetSetIconScreen(FALSE, wk->heapId);
    GFL_FadeSet(3, 16, 0, 2);
}

static BOOL PokemonTrade_StartRequest4(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 4);
    PokemonTrade_SetupMcssPokemon(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Start);
    return ret;
}

static BOOL PokemonTrade_StartRequest7(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0x96000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 7);
    PokemonTrade_SetupMcssPokemon(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeDemoDown_Start);
    return ret;
}

static BOOL PokemonTrade_StartRequest8(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0x96000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 8);
    PokemonTrade_SetupMcssPokemon(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Start);
    return ret;
}

static BOOL PokemonTrade_StartRequest6(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0x96000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 6);
    PokemonTrade_SetupMcssPokemon(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeDemoCommon_Start);
    return ret;
}

static BOOL PokemonTrade_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    u8 letter, shadow, background;
    int i;
    PokemonTradeWork *wk = work;
    BOOL done = TRUE;
    PokemonTradeState step = wk->state;
    if (BeaconStatus_IsLocked()) {
        return FALSE;
    }
    if (step != NULL) {
        step(wk);
        wk->timer++;
        done = FALSE;
        for (i = 0; i < 2; i++) {
            PoketradeNego_DrawOfferStatusLabel(wk, i, wk->unk11E8[i], 0);
        }
    }
    if (wk->unkF9C != 0 && wk->unk11F3 == 0) {
        PokemonTrade_SetSideMon(wk, 1, wk->pkm[wk->unkF9C - 1]);
        wk->unkF9C = 0;
    }
    PokemonTrade_UpdateBGM(wk);
    PoketradeNego_TickMarkerTimers(wk);
    ClActSys_Draw();
    if (wk->menu != NULL) {
        AppTaskMenu_Update(wk->menu);
    }
    if (wk->menuWin != NULL) {
        AppTaskMenuWin_Update(wk->menuWin);
    }
    if (wk->unk11FB_4) {
        if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK17, 0, NULL)) {
            wk->unk11FB_4 = 0;
        }
    }
    if (wk->tcbEx != NULL) {
        GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
        GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
        GFL_TCBExMgrUpdate(wk->tcbEx);
        GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
        PrintQueue_Main(wk->printQueue);
    }
    PoketradeNego_TickOfferSlotTimers(wk);
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(wk->camera);
    GFL_G3DSysMtxViewFlush();
    MCSSSys_Update(wk->mcssSys);
    MCSSSys_Draw(wk->mcssSys);
    Poketrade3D_Update3DScene(wk);
    GFL_G3DSysReqSwapBuffers();
    if (PokemonTrade_IsNetwork(wk) && wk->netSave == NULL && GFL_NetErrCheck()) {
        if (wk->unk11F7) {
            GFL_NetErrShowWifi(0x29);
        }
        if (wk->unk0 != NULL) {
            NHttpRap_EndRequest(wk->unk0);
            NHttpRap_Destroy(wk->unk0);
            wk->unk0 = NULL;
            DWCRap_SetErrorFunc(NULL, NULL);
            DWCRapCommon_EndSubHeap();
        }
        if (GFL_NetIsWifi()) {
            DWCRapCommon_CheckError(1, 1);
        } else if (wk->type != 1) {
            GFL_NetErrMarkShown();
        }
        wk->param->next = 2;
        PokemonTrade_RestoreBGM(wk);
        done = TRUE;
        Wipe_SetScreenCovered(0, 0);
        Wipe_SetScreenCovered(1, 0);
    }
    return done;
}

static BOOL PokemonTrade_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeWork *wk = work;
    int type = wk->type;
    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    GFL_HeapFree(wk->savedObjPalette);
    GFL_HeapFree(wk->savedBGPalette);
    Poketrade2D_DeleteMarkingIcon(&wk->infoIcons[0]);
    Poketrade2D_DeleteMarkingIcon(&wk->infoIcons[1]);
    Poketrade2D_DeleteMarkingIcon(&wk->infoIcons[2]);
    Poketrade2D_ClearSubScreen(wk);
    PokemonTrade_DeleteAllResources(wk);
    BeaconStatus_DoNothing();
    if (PokemonTrade_IsNetwork(wk)) {
        NetCommand_RemoveTable(TRADE_NET_CMD_BASE);
    }
    Poketrade3D_Delete3DScene(wk);
    GFL_HeapFree(wk->pkm[0]);
    GFL_HeapFree(wk->pkm[1]);
    GFL_TCBRemove(wk->vblankTcb);
    PokemonTrade_FreeMons(wk, param != NULL);
    PokemonTrade_ExitMcssSystem(wk);
    PokemonTrade_ExitDisplay(wk);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_IRC_BATTLE_MENU);
    GFL_HeapDelete(HEAPID_POKEMON_TRADE);
    if (type < 5) {
        GFL_OvlUnload(OVERLAY_ID(189));
        GFL_OvlUnload(OVERLAY_APP_UI);
    }
    if (type == 0 || type == 4) {
        GFL_OvlUnload(OVERLAY_ID(192));
    } else {
        GFL_OvlUnload(OVERLAY_ID(193));
    }
    return TRUE;
}

// Moves the cursor of the keys with a jump to another box
static void PokemonTrade_ClampScrollColumn(PokemonTradeWork *wk, BOOL moved) {
    if (moved) {
        int column = Poketrade2D_GetScrollColumn(wk) + 2;
        if (column >= wk->columnCount) {
            column -= wk->columnCount;
        }
        if (wk->scrollX == sBoxStartX[0]) {
            column = 2;
        } else if (wk->scrollX == sBoxStartX[wk->boxCount]) {
            column = 0;
            if (wk->cursorRow >= 3) {
                wk->cursorRow = 2;
            }
        }
        wk->cursorColumn = column;
        wk->firstColumn = -1;
    }
}

const GameProcFunctions data_ov194_021c63d0 = {
    PokemonTrade_StartRequest8,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c6400 = {
    PokemonTrade_StartRequest6,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions POKEMONTRADE_PROC_FUNCTIONS = {
    PokemonTrade_ProcInitGtsNego,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c63ac = {
    PokemonTrade_StartRequest4,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions POKEMONTRADE_WIFICLUB_PROC_FUNCTIONS = {
    PokemonTrade_ProcInitWifiClub,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c63dc = {
    PokemonTrade_ProcInitIrc,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c63b8 = {
    PokemonTrade_StartRequest7,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c640c = {
    PokemonTrade_StartRequest1,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};
