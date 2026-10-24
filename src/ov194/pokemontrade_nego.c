#include "types.h"
#include "app/pokemon_trade/pokemontrade_2d.h"
#include "app/pokemon_trade/pokemontrade_3d.h"
#include "app/pokemon_trade/pokemontrade_message.h"
#include "app/pokemon_trade/pokemontrade_nego.h"
#include "app/pokemon_trade/pokemontrade_proc.h"
#include "app/pokemon_trade/pokemontrade_touchbar.h"
#include "app/pokemon_trade_local.h"
#include "app/ui/touchbar.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/nhttp_rap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// Negotiating a trade: each player offers up to three Pokémon and chooses one of the other's, and a trade over the
// Wi-Fi Club checks the chosen Pokémon with the server first

// The message with the trainer name that a Pokémon gets when the server finds it invalid, which is the version's
#ifdef BLACK2
#define MSG_INVALID_PKM_TRAINER 25
#else
#define MSG_INVALID_PKM_TRAINER 24
#endif

static void PokemonTrade_SetSyncRange(NetHandle *handle, u32 timing, PokemonTradeWork *wk);
static void PokemonTrade_UpdatePickedIcon(PokemonTradeWork *wk);
static BOOL PokemonTrade_TickWifiTimer(PokemonTradeWork *wk);
static int PoketradeNego_CountOwnOffers(PokemonTradeWork *wk);
static int PoketradeNego_AddOffer(PokemonTradeWork *wk, int side, int slot, int box, PartyPkm *pkm);
static void PoketradeNego_StepQuitMenuChoice(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenQuitMenu(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitSync25(PokemonTradeWork *wk);
static void PoketradeNego_StepSendSync25(PokemonTradeWork *wk);
static void func_ov194_021bc938(PokemonTradeWork *wk);
static void PoketradeNego_StepResolveReplies(PokemonTradeWork *wk);
static void PoketradeNego_StepSendReply(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitReplySync(PokemonTradeWork *wk);
static void PoketradeNego_StepSendCancelReply(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitCancelSync(PokemonTradeWork *wk);
static void PoketradeNego_StepShowWaitMessageA(PokemonTradeWork *wk);
static void PoketradeNego_StepShowWaitMessageB(PokemonTradeWork *wk);
static void PoketradeNego_StepConfirmMenuChoice(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenConfirmMenu(PokemonTradeWork *wk);
static void PoketradeNego_StepResolveConfirm(PokemonTradeWork *wk);
static void PoketradeNego_StepSendConfirm(PokemonTradeWork *wk);
static void PoketradeNego_StepBeginConfirm(PokemonTradeWork *wk);
static void PoketradeNego_ShowOfferSummary(PokemonTradeWork *wk, int index);
static BOOL PoketradeNego_PollOfferCursorKeys(PokemonTradeWork *wk);
static void PoketradeNego_StepReturnFromSummary(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitSelectionDelay(PokemonTradeWork *wk);
static void PoketradeNego_StepBrowseOffers(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenSummaryFromBrowse(PokemonTradeWork *wk);
static void PoketradeNego_StepOfferActionChoice(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenOfferActionMenu(PokemonTradeWork *wk);
static void PoketradeNego_StepSendOfferMon(PokemonTradeWork *wk);
static void PoketradeNego_StepSendOfferIndex(PokemonTradeWork *wk);
static BOOL PoketradeNego_TrySelectOffer(PokemonTradeWork *wk, int index);
static void PoketradeNego_StepRebuildSceneAfterWipe(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitMessageThenWipe(PokemonTradeWork *wk);
static void PoketradeNego_StepSendDeclineConfirm(PokemonTradeWork *wk);
static void PoketradeNego_StepBeginDeclineConfirm(PokemonTradeWork *wk);
static void PoketradeNego_StepDeclineMenuChoice(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenDeclineMenu(PokemonTradeWork *wk);
static void PoketradeNego_StepBrowseOffersMain(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitInputAfterMessage(PokemonTradeWork *wk);
static void PoketradeNego_StepShowNoOfferMessage(PokemonTradeWork *wk);
static void PoketradeNego_SetupWindowRegsOffers(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitWipeBrowse(PokemonTradeWork *wk);
static void PoketradeNego_StepBuildBrowseScene(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitWipeToBrowse(PokemonTradeWork *wk);
static void PoketradeNego_StepWipeToBrowse(PokemonTradeWork *wk);
static void PoketradeNego_StepSendReady(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitReadySync(PokemonTradeWork *wk);
static void PoketradeNego_StepSendCheckResult(PokemonTradeWork *wk);
static void PoketradeNego_AbortHttpRequest(void *work, int a1, int code, int error);
static void PoketradeNego_StepPollMonCheck(PokemonTradeWork *wk);
static void PoketradeNego_StepStartMonCheck(PokemonTradeWork *wk);
static void PoketradeNego_StepRetryMonCheck(PokemonTradeWork *wk);
static void PoketradeNego_StepBeginMonCheckWipe(PokemonTradeWork *wk);
static void PoketradeNego_StepResolveFinalAgree(PokemonTradeWork *wk);
static void PoketradeNego_StepSendFinalAgree(PokemonTradeWork *wk);
static void PoketradeNego_StepSendFinalReady(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitMessageThenContinue(PokemonTradeWork *wk);
static void PoketradeNego_StepBeginFinalReady(PokemonTradeWork *wk);
static void PoketradeNego_StepFinalAgreeMenuChoice(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenFinalAgreeMenu(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitFinalMessage(PokemonTradeWork *wk);
static void PoketradeNego_StepBeginFinalWait(PokemonTradeWork *wk);
static void PoketradeNego_StepCancelMenuChoice(PokemonTradeWork *wk);
static void PoketradeNego_StepOpenCancelMenu(PokemonTradeWork *wk);
static void PoketradeNego_StepWaitTouchRelease(PokemonTradeWork *wk);
static BOOL PoketradeNego_SendOfferMon(PokemonTradeWork *wk, PartyPkm *pkm, int command);

// The six Pokémon on the screen, the three of each player
static TouchRect sNegoPkmRects[] = {
    { 0x18, 0x44, 0x08, 0x78 }, { 0x48, 0x74, 0x08, 0x78 }, { 0x78, 0xa4, 0x08, 0x78 }, { 0x18, 0x44, 0x88, 0xf8 },
    { 0x48, 0x74, 0x88, 0xf8 }, { 0x78, 0xa4, 0x88, 0xf8 }, { TOUCH_RECT_END },
};

static void PokemonTrade_SetSyncRange(NetHandle *handle, u32 timing, PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        NetHandle_SendTimingPair(handle, timing, 8);
    }
}

static void PokemonTrade_UpdatePickedIcon(PokemonTradeWork *wk) {
    if (GCTX_HIDIsUsingTouch()) {
        Poketrade2D_MovePickedIconActor(wk, -1);
    } else if (wk->cursor != -1) {
        Poketrade2D_MovePickedIconActor(wk, wk->cursor);
    }
}

BOOL PokemonTrade_CanTradeMon(PokemonTradeWork *wk, BoxPkm *pkm) {
    // The fields of a Pokémon that keep it from a negotiated trade
    static const u32 sBlockedFields[] = {
        0x69, 0x6a, 0x6b, 0x6c, 0x33, 0x34, 0x2c, 0x2f, 0x30, 0x31, 0x32, 0x66, 0x67, 0x68, 0x2e,
    };
    u32 species;
    BOOL wasEncrypted;
    BOOL found;
    u32 i = 0;
    species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    if (wk->type == 3) {
        wasEncrypted = PML_PkmDecrypt(pkm);
        for (; i < NELEMS(sBlockedFields); i++) {
            found = PML_PkmGetParam(pkm, sBlockedFields[i], NULL);
            if (found == TRUE) {
                break;
            }
        }
        PML_PkmReEncrypt(pkm, wasEncrypted);
        if (found) {
            return TRUE;
        }
        if (PML_PkmIsLegendNational(species)) {
            return TRUE;
        }
        if (hasPokemonChangedForm(pkm)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL PokemonTrade_TickWifiTimer(PokemonTradeWork *wk) {
    if (wk->waitTimer != 0) {
        if (PokemonTrade_IsWifiMode(wk)) {
            wk->waitTimer--;
        } else {
            wk->waitTimer = 0;
        }
    }
    if (wk->waitTimer == 0) {
        return TRUE;
    }
    return FALSE;
}

void PoketradeNego_FreeOfferMons(PokemonTradeWork *wk) {
    int side, i;
    for (side = 0; side < 2; side++) {
        for (i = 0; i < 3; i++) {
            if (wk->negoPkm[side][i] != NULL) {
                GFL_HeapFree(wk->negoPkm[side][i]);
                wk->negoPkm[side][i] = NULL;
            }
        }
    }
}

void PoketradeNego_ClearOfferSlots(PokemonTradeWork *wk) {
    int side, i;
    for (side = 0; side < 2; side++) {
        for (i = 0; i < 3; i++) {
            wk->negoSlot[side][i] = -1;
            wk->negoBox[side][i] = -1;
            if (wk->negoPkm[side][i] != NULL) {
                MI_CpuFill8(wk->negoPkm[side][i], 0, PokeParty_GetPkmRawSize());
            }
        }
    }
}

// The number of Pokémon this player offers
static int PoketradeNego_CountOwnOffers(PokemonTradeWork *wk) {
    int i;
    int count = 0;
    for (i = 0; i < 3; i++) {
        if (wk->negoSlot[0][i] != -1 && wk->negoBox[0][i] != -1) {
            count++;
        }
    }
    return count;
}

BOOL PoketradeNego_CheckCancelTouch(PokemonTradeWork *wk) {
    if (wk->type == 3) {
        if (wk->menuWin == NULL) {
            PoketradeSave_OpenTaskMenuWindow(wk, 155);
        }
        if (PoketradeSave_IsTouchOutsideArea(wk)) {
            PokemonTrade_ClearNetAutoExit(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return TRUE;
        }
    }
    return FALSE;
}

void PoketradeNego_CloseCancelMenu(PokemonTradeWork *wk, u32 a1, u32 a2) {
    if (wk->type == 3) {
        PoketradeSave_CloseTaskMenuWindow(wk, a1, a2);
    }
}

BOOL PoketradeNego_CanConfirmOffers(PokemonTradeWork *wk) {
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(wk->gameData);
    int offered = PoketradeNego_CountOwnOffers(wk);
    int count = PokeParty_GetPkmCount(GameData_GetParty(wk->gameData)) + howManyTotalPokesAreInBoxes(boxes);
    if (count == 2 && offered >= 1) {
        return TRUE;
    }
    if (count > 2 && offered >= 2) {
        return TRUE;
    }
    return FALSE;
}

BOOL PoketradeNego_HasOffer(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 3; i++) {
            if (wk->negoSlot[0][i] != -1 && wk->negoBox[0][i] != -1) {
                return TRUE;
            }
        }
    } else if (wk->selectSlot != -1 && wk->selectBox != -1) {
        return TRUE;
    }
    return FALSE;
}

int PoketradeNego_FindOfferIndex(PokemonTradeWork *wk, int box, int slot) {
    int i;
    for (i = 0; i < 3; i++) {
        if (slot == wk->negoSlot[0][i] && box == wk->negoBox[0][i]) {
            return i;
        }
    }
    return -1;
}

void PoketradeNego_StoreRemoteOffer(PokemonTradeWork *wk, int index, PartyPkm *pkm) {
    wk->negoSlot[1][index] = index;
    wk->negoBox[1][index] = index;
    if (wk->negoPkm[1][index] == NULL) {
        wk->negoPkm[1][index] =
            GFL_HeapAllocate(wk->heapId, PokeParty_GetPkmRawSize(), TRUE, "pokemontrade_nego.c", 293);
    }
    MI_CpuCopy8(pkm, wk->negoPkm[1][index], PokeParty_GetPkmRawSize());
}

// The free place among a side's offers for a Pokémon, or -1 if it is offered already
int PoketradeNego_FindFreeOfferIndex(PokemonTradeWork *wk, int side, int slot, int box) {
    int i;
    for (i = 0; i < 3; i++) {
        if (slot == wk->negoSlot[side][i] && box == wk->negoBox[side][i]) {
            return -1;
        }
    }
    for (i = 0; i < 3; i++) {
        if (wk->negoSlot[side][i] == -1) {
            return i;
        }
    }
    return 2;
}

static int PoketradeNego_AddOffer(PokemonTradeWork *wk, int side, int slot, int box, PartyPkm *pkm) {
    int i;
    for (i = 0; i < 3; i++) {
        if (slot == wk->negoSlot[side][i] && box == wk->negoBox[side][i]) {
            return -1;
        }
    }
    for (i = 0; i < 3; i++) {
        if (wk->negoSlot[side][i] == -1 || i == 2) {
            wk->negoSlot[side][i] = slot;
            wk->negoBox[side][i] = box;
            if (wk->negoPkm[side][i] == NULL) {
                wk->negoPkm[side][i] =
                    GFL_HeapAllocate(wk->heapId, PokeParty_GetPkmRawSize(), TRUE, "pokemontrade_nego.c", 335);
            }
            MI_CpuCopy8(pkm, wk->negoPkm[side][i], PokeParty_GetPkmRawSize());
            PoketradeNego_ShowOfferSlotMon(wk, side, pkm);
            return i;
        }
    }
    return -1;
}

void PoketradeNego_RemoveOffer(PokemonTradeWork *wk, int side, int index) {
    wk->negoSlot[side][index] = -1;
    wk->negoBox[side][index] = -1;
    if (wk->negoPkm[side][index] != NULL) {
        GFL_HeapFree(wk->negoPkm[side][index]);
    }
    wk->negoPkm[side][index] = NULL;
}

void PoketradeNego_RegisterIconResources(PokemonTradeWork *wk, u32 vram) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(7, wk->heapId);
    u16 offset = vram == 1 ? 0x60 : 0;
    wk->objRes[TRADE_OBJRES_PLTT_NEGO] = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), vram, offset, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_NEGO] =
        ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_MainEng(), wk->heapId);
    GFL_ArcToolFree(arc);
}

void PoketradeNego_ReleaseIconResources(PokemonTradeWork *wk) {
    if (wk->objRes[TRADE_OBJRES_PLTT_NEGO] != 0) {
        ClActSys_FreePltt(wk->objRes[TRADE_OBJRES_PLTT_NEGO]);
        wk->objRes[TRADE_OBJRES_PLTT_NEGO] = 0;
    }
    if (wk->objRes[TRADE_OBJRES_CELL_NEGO] != 0) {
        ClActSys_FreeCellAnims(wk->objRes[TRADE_OBJRES_CELL_NEGO]);
        wk->objRes[TRADE_OBJRES_CELL_NEGO] = 0;
    }
}

BOOL PoketradeNego_AddSelectedAsOffer(PokemonTradeWork *wk, int slot, int box) {
    PartyPkm *pkm;
    int index;
    NetHandle_GetOwn();
    pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->selectBox, wk->selectSlot, wk);
    if (PokemonTrade_IsNetwork(wk)) {
        u32 i = 0;
        index = PoketradeNego_AddOffer(wk, 0, slot, box, pkm);
        if (index != -1) {
            if (!PoketradeNego_SendOfferMon(wk, wk->negoPkm[0][index], TRADE_NET_CMD_OFFER + index)) {
                PoketradeNego_RemoveOffer(wk, 0, index);
                GFL_HeapFree(pkm);
                return FALSE;
            }
            // The data was printed here
            for (i = 0; i < PokeParty_GetPkmRawSize(); i++) {
            }
        }
    } else {
        index = PoketradeNego_AddOffer(wk, 0, slot, box, pkm);
    }
    if (index == -1) {
        GFL_HeapFree(pkm);
        return TRUE;
    }
    Poketrade2D_CreateOfferIconActor(wk, 0, index, pkm, 1, 0);
    GFL_HeapFree(pkm);
    return TRUE;
}

void PoketradeNego_DrawPlayerNames(PokemonTradeWork *wk) {
    u32 xs[] = { 1, 17 };
    PlayerInfo *infos[2];
    BmpWin *window;
    int i;
    infos[0] = wk->myInfo;
    infos[1] = wk->partnerInfo;
    GFL_BGSysLoadNCLRDefault(getUINarcIdx(), 31, 0, 0x1a0, 0x20, wk->heapId);
    if (wk->unk5D4 != NULL) {
        BmpWin_Free(wk->unk5D4);
        BmpWin_Free(wk->unk5D8);
    }
    window = BmpWin_CreateDynamic(3, 0, 0, 32, 2, 13, FALSE);
    wk->unk5D4 = window;
    GFL_TextRndUpdateColorIndexLUT(14, 15, 0);
    if (wk->type == 3 && wk->unk1050[0] != -1) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 134, wk->drawStr);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 8, 2, wk->drawStr, wk->font);
        GFL_MsgDataLoadStrbuf(wk->msgData, wk->unk1050[0] + 124, wk->drawStr);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 136, 2, wk->drawStr, wk->font);
    }
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    window = BmpWin_CreateDynamic(3, 0, 3, 32, 8, 5, FALSE);
    wk->unk5D8 = window;
    for (i = 0; i < 2; i++) {
        if (infos[i] != NULL) {
            int x;
            int msg;
            if (wk->type == 3 && wk->unk1048[i] != -1) {
                msg = 118;
            } else {
                msg = 129;
            }
            GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
            GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->drawTemplate);
            copyVarForText(wk->wordSet, 0, infos[i]);
            GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
            x = xs[i] * 8;
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, 2, wk->drawStr, wk->font);
            if (wk->type == 3 && wk->unk1048[i] != -1) {
                GFL_TextRndUpdateColorIndexLUT(3, 4, 0);
                GFL_MsgDataLoadStrbuf(wk->msgData, wk->unk1048[i] + 119, wk->drawStr);
                GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x + 4, 34, wk->drawStr, wk->font);
            }
        }
    }
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(3);
}

void PoketradeNego_DrawOfferStatusLabel(PokemonTradeWork *wk, int side, u32 msg, BOOL force) {
    u32 xs[] = { 2, 19 };
    BmpWin *window;
    if (force || (wk->unk11E8[side] != 3 && wk->unk11EA && PokemonTrade_IsNegoType(wk))) {
        if (wk->unk5BC[side] != NULL) {
            BmpWin_Free(wk->unk5BC[side]);
        }
        wk->unk5BC[side] = BmpWin_CreateDynamic(3, xs[side], 16, 12, 2, 5, FALSE);
        window = wk->unk5BC[side];
        GFL_TextRndUpdateColorIndexLUT(5, 6, 0);
        msg += 115;
        GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->drawStr);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, wk->drawStr, wk->font);
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysQueueScrLoad(3);
        wk->unk11E8[side] = 3;
    }
}

void PoketradeNego_DeleteHeaderWindows(PokemonTradeWork *wk) {
    if (wk->unk5BC[0] != NULL) {
        BmpWin_ClearScreen(wk->unk5BC[0]);
        BmpWin_Free(wk->unk5BC[0]);
        wk->unk5BC[0] = NULL;
    }
    if (wk->unk5BC[1] != NULL) {
        BmpWin_ClearScreen(wk->unk5BC[1]);
        BmpWin_Free(wk->unk5BC[1]);
        wk->unk5BC[1] = NULL;
    }
    if (wk->unk5D4 != NULL) {
        BmpWin_ClearScreen(wk->unk5D4);
        BmpWin_Free(wk->unk5D4);
        wk->unk5D4 = NULL;
    }
    if (wk->unk5D8 != NULL) {
        BmpWin_ClearScreen(wk->unk5D8);
        BmpWin_Free(wk->unk5D8);
        wk->unk5D8 = NULL;
    }
    GFL_BGSysQueueScrLoad(3);
}

static void PoketradeNego_StepQuitMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
            PokemonTrade_SetState(wk, PoketradeNego_StepOpenSummaryFromBrowse);
            break;
        case 1:
            PokemonTrade_ResetTouchbarBackButton(wk);
            PoketradeNego_SetupWindowRegsOffers(wk);
            PokemonTrade_UpdatePickedIcon(wk);
            PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffersMain);
            break;
        }
    }
}

static void PoketradeNego_StepOpenQuitMenu(PokemonTradeWork *wk) {
    u32 items[] = { 3, 5 };
    PoketradeSave_OpenTaskMenu(wk, items, NELEMS(items));
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
    PokemonTrade_SetState(wk, PoketradeNego_StepQuitMenuChoice);
}

static void PoketradeNego_StepWaitSync25(PokemonTradeWork *wk) {
    NetHandle_GetOwn();
    if (PokemonTrade_IsNetwork(wk) && !NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x19, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeSave_CloseMessageWindowClear(wk);
    PoketradeNego_CloseCancelMenu(wk, 0, 0);
    PokemonTrade_ResetTouchbarBackButton(wk);
    PokemonTrade_UpdatePickedIcon(wk);
    PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffersMain);
}

static void PoketradeNego_StepSendSync25(PokemonTradeWork *wk) {
    NetHandle_GetOwn();
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        PokemonTrade_SetSyncRange(NetHandle_GetOwn(), 0x19, wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitSync25);
    }
}

static void func_ov194_021bc938(PokemonTradeWork *wk) {
    NetHandle_GetOwn();
    if (PoketradeSave_IsMessagePrintDone(wk) && (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PoketradeSave_ShowTimeIcon(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepSendSync25);
    }
}

static void PoketradeNego_StepResolveReplies(PokemonTradeWork *wk) {
    int msg;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    int mine, other;
    if (!PoketradeSave_IsMessagePrintDone(wk) || !PokemonTrade_TickWifiTimer(wk)) {
        return;
    }
    if (PokemonTrade_IsNetwork(wk) && !NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x1d, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 0, 0);
    if (wk->command[myId] == 13 && wk->command[otherId] == 13) {
        PokemonTrade_SetSyncRange(NetHandle_GetOwn(), 13, wk);
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
    } else if (other == 6) {
        msg = 97;
    } else if (mine == 6) {
        msg = 112;
    } else if (other == 14) {
        msg = 152;
    } else if (mine == 14) {
        msg = 151;
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    PokemonTrade_SetState(wk, func_ov194_021bc938);
}

static void PoketradeNego_StepSendReply(PokemonTradeWork *wk) {
    u8 command = 13;
    if (PokemonTrade_IsSelfMonReady(wk)) {
        command = 5;
    } else if (PokemonTrade_IsCurrentSideMonInvalid(wk)) {
        command = 7;
    } else if (PokemonTrade_IsOtherSideMonInvalid(wk)) {
        command = 8;
    } else if (PokemonTrade_IsMonTradeable(wk)) {
        command = 6;
    }
    if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_SELECT, 1, &command)) {
        PokemonTrade_SetSyncRange(NetHandle_GetOwn(), 0x1d, wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveReplies);
    }
}

static void PoketradeNego_StepWaitReplySync(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk)) {
        wk->command[0] = 13;
        wk->command[1] = 13;
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveReplies);
        return;
    }
    if (!NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x1e, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 0, 0);
    PokemonTrade_SetState(wk, PoketradeNego_StepSendReply);
}

static void PoketradeNego_StepSendCancelReply(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSelect(14)) {
        PokemonTrade_SetSyncRange(NetHandle_GetOwn(), 0x1d, wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveReplies);
    }
}

static void PoketradeNego_StepWaitCancelSync(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk)) {
        wk->command[0] = 14;
        wk->command[1] = 14;
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveReplies);
        return;
    }
    if (!NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x1e, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 0, 0);
    PokemonTrade_SetState(wk, PoketradeNego_StepSendCancelReply);
}

static void PoketradeNego_StepShowWaitMessageA(PokemonTradeWork *wk) {
    PokemonTrade_SetSyncRange(NetHandle_GetOwn(), 0x1e, wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, PoketradeNego_StepWaitReplySync);
}

static void PoketradeNego_StepShowWaitMessageB(PokemonTradeWork *wk) {
    PokemonTrade_SetSyncRange(NetHandle_GetOwn(), 0x1e, wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, PoketradeNego_StepWaitCancelSync);
}

static void PoketradeNego_StepConfirmMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, PoketradeNego_StepShowWaitMessageA);
            break;
        case 1:
            PokemonTrade_SetState(wk, PoketradeNego_StepShowWaitMessageB);
            break;
        }
    }
}

static void PoketradeNego_StepOpenConfirmMenu(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        u32 items[] = { 4, 5 };
        PoketradeSave_OpenTaskMenu(wk, items, NELEMS(items));
        GFL_BGSysSetEnabledBGsA(0x1f);
        TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
        PokemonTrade_SetState(wk, PoketradeNego_StepConfirmMenuChoice);
    }
}

static void PoketradeNego_StepResolveConfirm(PokemonTradeWork *wk) {
    int msg;
    int i;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    if (!PoketradeSave_IsMessagePrintDone(wk) || !PokemonTrade_TickWifiTimer(wk)) {
        return;
    }
    if (PokemonTrade_IsNetwork(wk) && !NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x1c, 8)) {
        PoketradeNego_CheckCancelTouch(wk);
        return;
    }
    PoketradeNego_CloseCancelMenu(wk, 0, 0);
    if (wk->command[myId] == 11 && wk->command[otherId] == 11) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 147, wk->strbufTemplate);
        for (i = 0; i < 2; i++) {
            loadPokemonNicknameToStrbuf(wk->wordSet, i, PokemonTrade_GetPkm(wk, i));
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
        PoketradeSave_OpenMessageWindowTop(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepOpenConfirmMenu);
        return;
    }
    if (wk->command[otherId] == 12) {
        wk->waitTimer = 120;
        msg = 148;
        if (wk->type == 3 && wk->checkCount >= 3) {
            PokemonTrade_ClearNetAutoExit(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return;
        }
    } else if (wk->command[myId] == 12) {
        wk->waitTimer = 120;
        msg = 150;
        if (wk->type == 3 && wk->checkCount >= 3) {
            PokemonTrade_ClearNetAutoExit(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return;
        }
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    if (PokemonTrade_IsNetwork(wk)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x19, 8);
    }
    wk->timer = 0;
    PokemonTrade_SetState(wk, PoketradeNego_StepWaitMessageThenWipe);
}

static void PoketradeNego_StepSendConfirm(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSelect(11)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1c, 8);
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveConfirm);
    }
}

static void PoketradeNego_StepBeginConfirm(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    wk->waitTimer = 120;
    if (!PokemonTrade_IsNetwork(wk)) {
        PartyPkm *pkm;
        u32 size;
        wk->command[0] = 11;
        wk->command[1] = 11;
        pkm = wk->negoPkm[0][0];
        PokemonTrade_SetSideMon(wk, 1, pkm);
        size = PokeParty_GetPkmRawSize();
        MI_CpuCopy8(pkm, PokemonTrade_GetPkm(wk, 1), size);
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveConfirm);
    } else {
        PokemonTrade_SetState(wk, PoketradeNego_StepSendConfirm);
    }
}

// Shows the other player's Pokémon across from the one chosen
static void PoketradeNego_ShowOfferSummary(PokemonTradeWork *wk, int index) {
    int slot = index % 3;
    int side = index / 3;
    int other = 1 - side;
    PartyPkm *pkm = wk->negoPkm[other][slot];
    if (PokemonTrade_IsMonValid((u8 *)pkm)) {
        PoketradeSave_ShowSummaryMon(wk, pkm, other, 1);
        Poketrade2D_UpdateOfferIconHighlight(wk, side, slot);
    }
}

static BOOL PoketradeNego_PollOfferCursorKeys(PokemonTradeWork *wk) {
    BOOL moved = FALSE;
    if (PokemonTrade_GetPressedKeys() == PAD_KEY_RIGHT) {
        wk->cursor = (wk->cursor + 3) % 6;
        moved = TRUE;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_KEY_LEFT) {
        wk->cursor = (wk->cursor - 3) % 6;
        if (wk->cursor < 0) {
            wk->cursor += 6;
        }
        moved = TRUE;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_KEY_UP) {
        if (wk->cursor == 0 || wk->cursor == 3) {
            wk->cursor += 3;
        }
        wk->cursor--;
        moved = TRUE;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_KEY_DOWN) {
        if (wk->cursor == 2 || wk->cursor == 5) {
            wk->cursor -= 3;
        }
        wk->cursor++;
        moved = TRUE;
    }
    return moved;
}

static void PoketradeNego_StepReturnFromSummary(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        ArcTool *arc;
        GFL_BGSysSetBGEnabled(3, FALSE);
        Poketrade2D_UpdateOfferIconHighlight(wk, -1, -1);
        PoketradeSave_CloseSummaryPage(wk, 0);
        arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);
        // The library takes the palette offset as a u16, which the prototype doesn't say
        GFL_G2DIOLoadNSCRSync(arc, 11, 2, 0, (u16)wk->bg2Chars, 0, FALSE, wk->heapId);
        GFL_ArcToolFree(arc);
        Poketrade3D_AddUnitAndInit(wk, 0);
        GFL_BGSysQueueScrLoad(3);
        wk->unk11F3 = 0;
        if (PokemonTrade_IsMonValid((u8 *)PokemonTrade_GetPkm(wk, 1))) {
            PokemonTrade_SetSideMon(wk, 1, PokemonTrade_GetPkm(wk, 1));
        }
        if (PokemonTrade_IsMonValid((u8 *)PokemonTrade_GetPkm(wk, 0))) {
            PokemonTrade_SetSideMon(wk, 0, PokemonTrade_GetPkm(wk, 0));
        }
        PoketradeNego_SetupWindowRegsOffers(wk);
        PokemonTrade_UpdatePickedIcon(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffersMain);
        PokemonTrade_ResetTouchbarBackButton(wk);
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
    }
}

static void PoketradeNego_StepWaitSelectionDelay(PokemonTradeWork *wk) {
    int wait = 3;
    if (GFL_NetIsWifi()) {
        wait = 10;
    }
    if (wk->timer > wait) {
        PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffers);
    }
}

static void PoketradeNego_StepBrowseOffers(PokemonTradeWork *wk) {
    int index;
    GFL_BGSysSetBGEnabled(3, TRUE);
    if (!GFL_WipeIsFinished()) {
        return;
    }
    TouchBar_Main(wk->touchBar);
    if (TouchBar_GetDecided(wk->touchBar) == TOUCHBAR_ICON_RETURN) {
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, PoketradeNego_StepReturnFromSummary);
        return;
    }
    if (TouchBar_GetTouched(wk->touchBar) != TOUCHBAR_ICON_NONE) {
        return;
    }
    index = GFL_HIDGetTouchPressedRect(sNegoPkmRects);
    if (index != TOUCH_RECT_NONE) {
        GCTX_HIDSetUsingTouch(TRUE);
        wk->cursor = index;
        Poketrade2D_MovePickedIconActor(wk, -1);
        PoketradeNego_ShowOfferSummary(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->timer = 0;
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitSelectionDelay);
        return;
    }
    if (GCTX_HIDIsUsingTouch() && PokemonTrade_GetPressedKeys()) {
        GCTX_HIDSetUsingTouch(FALSE);
        Poketrade2D_MovePickedIconActor(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->timer = 0;
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitSelectionDelay);
        return;
    }
    if (PoketradeNego_PollOfferCursorKeys(wk)) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Poketrade2D_MovePickedIconActor(wk, wk->cursor);
        PoketradeNego_ShowOfferSummary(wk, wk->cursor);
        wk->timer = 0;
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitSelectionDelay);
    }
}

static void PoketradeNego_StepOpenSummaryFromBrowse(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        u16 index = wk->cursor;
        int slot = index % 3;
        int side = index / 3;
        PartyPkm *pkm = wk->negoPkm[1 - side][slot];
        wk->unk11F3 = 1;
        if (pkm != NULL && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            PoketradeSave_OpenSummaryScreen(wk, pkm);
            Poketrade2D_UpdateOfferIconHighlight(wk, side, slot);
        }
        PoketradeNego_SetupWindowRegsOffers(wk);
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
        PokemonTrade_ResetTouchbarBackButton(wk);
        PokemonTrade_UpdatePickedIcon(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffers);
    }
}

static void PoketradeNego_StepOfferActionChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, PoketradeNego_StepBeginConfirm);
            break;
        case 1:
            GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
            TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
            PokemonTrade_SetState(wk, PoketradeNego_StepOpenSummaryFromBrowse);
            break;
        case 2:
            PoketradeNego_SetupWindowRegsOffers(wk);
            PokemonTrade_ResetTouchbarBackButton(wk);
            PokemonTrade_UpdatePickedIcon(wk);
            PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffersMain);
            break;
        }
    }
}

static void PoketradeNego_StepOpenOfferActionMenu(PokemonTradeWork *wk) {
    u32 items[] = { 4, 3, 5 };
    PoketradeSave_OpenTaskMenu(wk, items, NELEMS(items));
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
    PokemonTrade_SetState(wk, PoketradeNego_StepOfferActionChoice);
}

static void PoketradeNego_StepSendOfferMon(PokemonTradeWork *wk) {
    if (wk->unk11FB_0 != 1) {
        if (!PokemonTrade_IsNetwork(wk) ||
            GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK1, PokeParty_GetPkmRawSize(), wk->recvPkm[2])) {
            wk->unk11FB_0 = 1;
            PokemonTrade_SetState(wk, PoketradeNego_StepOpenOfferActionMenu);
        }
    }
}

static void PoketradeNego_StepSendOfferIndex(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        u8 inParty = PokemonTrade_GetSideValue(wk, 0);
        // BUG: The command's receiver reads one byte, but a Pokémon's size is sent from the one-byte local
#ifdef BUGFIX
        if (!GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK16, sizeof(inParty), &inParty)) {
#else
        if (!GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK16, PokeParty_GetPkmRawSize(), &inParty)) {
#endif
            return;
        }
    }
    PokemonTrade_SetState(wk, PoketradeNego_StepSendOfferMon);
}

// Chooses one of the six Pokémon, returning whether it is there
static BOOL PoketradeNego_TrySelectOffer(PokemonTradeWork *wk, int index) {
    int side = index / 3;
    int slot = index % 3;
    if (index != -1) {
        if (wk->negoSlot[1 - side][slot] != -1) {
            PartyPkm *pkm = wk->negoPkm[1 - side][slot];
            u32 value = wk->unkFF0[1 - side][slot];
            if (pkm != NULL && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                wk->cursor = index;
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
                if (side != 0) {
                    PokemonTrade_SetState(wk, PoketradeNego_StepOpenQuitMenu);
                } else {
                    PokemonTrade_SetSideMon(wk, side, pkm);
                    MI_CpuCopy8(pkm, wk->recvPkm[2], PokeParty_GetPkmRawSize());
                    PokemonTrade_SetSideValue(wk, 0, value);
                    PokemonTrade_SetState(wk, PoketradeNego_StepSendOfferIndex);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void PoketradeNego_StepRebuildSceneAfterWipe(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        PokemonTrade_DeleteAllResources(wk);
        Poketrade2D_SetDisplayConfig(0);
        Poketrade2D_SetupSubBgs(wk);
        wk->clactUnit = ClActUnit_Create(340, 0, wk->heapId);
        PoketradeSave_CreateMessageResources(wk);
        Poketrade2D_RegisterIconSpriteResources(wk);
        Poketrade2D_CreateBoxIconActors(wk);
        Poketrade2D_LoadBackgroundGfx(wk);
        Poketrade2D_CreateFontResource(wk);
        Poketrade2D_LoadMainGfxAndMessageWindow(wk);
        Poketrade3D_AddUnitAndInit(wk, 0);
        GFL_BGSysSetEnabledBGsA(0x1f);
        GFL_BGSysSetEnabledBGsB(0x1e);
        PokemonTrade_CreateTouchBar(wk);
        GFL_NetSetIconScreen(FALSE, wk->heapId);
        GXS_SetVisibleWnd(GX_WNDMASK_NONE);
        PokemonTrade_SetState(wk, PokemonTrade_StepResetForNewRound);
    }
}

static void PoketradeNego_StepWaitMessageThenWipe(PokemonTradeWork *wk) {
    NetHandle_GetOwn();
    if (PoketradeSave_IsMessagePrintDone(wk) && PokemonTrade_TickWifiTimer(wk) && wk->timer >= 120) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, PoketradeNego_StepRebuildSceneAfterWipe);
    }
}

static void PoketradeNego_StepSendDeclineConfirm(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSelect(12)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1c, 8);
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveConfirm);
    }
}

static void PoketradeNego_StepBeginDeclineConfirm(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, PoketradeNego_StepSendDeclineConfirm);
}

static void PoketradeNego_StepDeclineMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        if (choice == 0) {
            PokemonTrade_SetState(wk, PoketradeNego_StepBeginDeclineConfirm);
        } else {
            PoketradeSave_CloseMessageWindowClear(wk);
            PokemonTrade_ResetTouchbarBackButton(wk);
            PokemonTrade_UpdatePickedIcon(wk);
            PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffersMain);
        }
    }
}

static void PoketradeNego_StepOpenDeclineMenu(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        u32 items[] = { 24, 25 };
        PoketradeSave_OpenTaskMenuAt(wk, items, NELEMS(items), 32, 12);
        PokemonTrade_SetState(wk, PoketradeNego_StepDeclineMenuChoice);
    }
}

static void PoketradeNego_StepBrowseOffersMain(PokemonTradeWork *wk) {
    int index;
    TouchBar_Main(wk->touchBar);
    if (TouchBar_GetDecided(wk->touchBar) == TOUCHBAR_ICON_RETURN) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 149, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
        PokemonTrade_UpdatePickedIcon(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepOpenDeclineMenu);
        return;
    }
    if (TouchBar_GetTouched(wk->touchBar) != TOUCHBAR_ICON_NONE) {
        return;
    }
    index = GFL_HIDGetTouchPressedRect(sNegoPkmRects);
    if (index != TOUCH_RECT_NONE) {
        GCTX_HIDSetUsingTouch(TRUE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Poketrade2D_MovePickedIconActor(wk, -1);
        if (PoketradeNego_TrySelectOffer(wk, index)) {
            return;
        }
    }
    if (GCTX_HIDIsUsingTouch() && PokemonTrade_GetPressedKeys()) {
        wk->cursor %= 6;
        if (wk->cursor < 0) {
            wk->cursor = 0;
        }
        Poketrade2D_MovePickedIconActor(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_BUTTON_A && PoketradeNego_TrySelectOffer(wk, wk->cursor)) {
        return;
    }
    if (PoketradeNego_PollOfferCursorKeys(wk)) {
        Poketrade2D_MovePickedIconActor(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
}

static void PoketradeNego_StepWaitInputAfterMessage(PokemonTradeWork *wk) {
    if (PokemonTrade_TickWifiTimer(wk) && PoketradeSave_IsMessagePrintDone(wk) &&
        (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        if (GCTX_HIDIsUsingTouch()) {
            Poketrade2D_MovePickedIconActor(wk, -1);
        } else {
            Poketrade2D_MovePickedIconActor(wk, 0);
        }
        PoketradeSave_CloseMessageWindowClear(wk);
        PokemonTrade_ResetTouchbarBackButton(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepBrowseOffersMain);
    }
}

static void PoketradeNego_StepShowNoOfferMessage(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 146, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, PoketradeNego_StepWaitInputAfterMessage);
}

// Shows the upper screen's window over the other player's side
static void PoketradeNego_SetupWindowRegsOffers(PokemonTradeWork *wk) {
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x1b, -8);
    G2S_SetWnd0InsidePlane(0x1f, TRUE);
    G2S_SetWnd0Position(128, 0, 255, 169);
    G2S_SetWndOutsidePlane(0x1f, FALSE);
    GXS_SetVisibleWnd(GX_WNDMASK_W0);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
}

static void PoketradeNego_StepWaitWipeBrowse(PokemonTradeWork *wk) {
    if (Poketrade2D_StepCursorSlide(wk)) {
        if (GCTX_HIDIsUsingTouch()) {
            Poketrade2D_MovePickedIconActor(wk, -1);
            wk->cursor = 0;
        } else {
            Poketrade2D_MovePickedIconActor(wk, 0);
            wk->cursor = 0;
        }
        if (GFL_WipeIsFinished()) {
            PoketradeNego_SetupWindowRegsOffers(wk);
            PokemonTrade_SetState(wk, PoketradeNego_StepShowNoOfferMessage);
        }
    }
}

static void PoketradeNego_StepBuildBrowseScene(PokemonTradeWork *wk) {
    Poketrade2D_FreeMainResources(wk);
    PoketradeNego_UpdateOfferMarkers(wk);
    PoketradeNego_ResetMarkers(wk);
    Poketrade2D_DeleteAllOfferIconActors(wk);
    PoketradeNego_ReleaseIconResources(wk);
    Poketrade2D_DeleteBoxWindows(wk);
    PoketradeNego_DeleteHeaderWindows(wk);
    Poketrade2D_DeleteAllBoxIcons(wk);
    PoketradeNego_RegisterIconResources(wk, 1);
    GFL_BGSysFillScrAsync(6, 0);
    ClActor_SetVisible(wk->actors[2], FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_RETURN, TRUE);
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
    PoketradeSave_DrawOfferListWindows(wk, 6);
    wk->unk108C = 0;
    wk->unk1084 = 1;
    Poketrade2D_SetupCursorBg(wk);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, PoketradeNego_StepWaitWipeBrowse);
}

static void PoketradeNego_StepWaitWipeToBrowse(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        PokemonTrade_SetState(wk, PoketradeNego_StepBuildBrowseScene);
    }
}

static void PoketradeNego_StepWipeToBrowse(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || wk->unk1060 != 0) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitWipeToBrowse);
    }
}

static void PoketradeNego_StepSendReady(PokemonTradeWork *wk) {
    NetHandle *handle = NetHandle_GetOwn();
    if (PokemonTrade_IsNetwork(wk)) {
        u8 data = 1;
        if (GFL_NetSendCommand(handle, TRADE_NET_CMD_UNKA, 1, &data)) {
            PokemonTrade_SetState(wk, PoketradeNego_StepWipeToBrowse);
        }
    } else {
        PokemonTrade_SetState(wk, PoketradeNego_StepWipeToBrowse);
    }
}

static void PoketradeNego_StepWaitReadySync(PokemonTradeWork *wk) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x15, 8)) {
        int msg;
        u8 result = wk->checkResult;
        if (result == 0 && wk->partnerCheckResult == 0) {
            PokemonTrade_SetState(wk, PoketradeNego_StepSendReady);
            return;
        }
        if (result == 0xf0) {
            msg = 159;
        } else if (result == 0xf1) {
            msg = 160;
        } else if (result == 0xf2) {
            msg = 161;
        } else if (result == 0xff) {
            msg = 162;
        } else if ((wk->partnerCheckResult & 0xf0) == 0xf0) {
            msg = 162;
        } else if (wk->partnerCheckResult != 0) {
            msg = 144;
        } else {
            msg = 145;
        }
        GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        wk->waitTimer = 120;
        wk->timer = 0;
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitMessageThenWipe);
    }
}

static void PoketradeNego_StepSendCheckResult(PokemonTradeWork *wk) {
    NetHandle *handle = NetHandle_GetOwn();
    wk->unk109A = 0;
    if (GFL_NetSendCommand(handle, TRADE_NET_CMD_CHECK_RESULT, 1, &wk->checkResult)) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x15, 8);
        if (wk->type == 2) {
            DWCRap_SetMic(FALSE);
        }
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitReadySync);
    }
}

static void PoketradeNego_AbortHttpRequest(void *work, int a1, int code, int error) {
    PokemonTradeWork *wk = work;
    if (wk->unk0 != NULL) {
        NHttpRap_EndRequest(wk->unk0);
        NHttpRap_Destroy(wk->unk0);
        wk->unk0 = NULL;
        DWCRapCommon_EndSubHeap();
    }
}

// Waits for the server's check of the other player's Pokémon
static void PoketradeNego_StepPollMonCheck(PokemonTradeWork *wk) {
    int i;
    u8 count = 0;
    void *response;
    int status = NHttpRap_GetStatus(wk->unk0);
    int result = NHttpRap_Poll(wk->unk0);
    if (result == 0 && status == 200) {
        response = NHttpRap_GetAnswer(wk->unk0);
        wk->checkResult = NHttpRap_GetCheckStatus(response);
        if (wk->checkResult == 1 && wk->unk109A == 0) {
            for (i = 0; i < 3; i++) {
                if (PokemonTrade_IsMonValid((u8 *)wk->negoPkm[1][i])) {
                    PartyPkm *pkm = wk->negoPkm[1][i];
                    if (NHttpRap_GetCheckResult(response, count)) {
                        u32 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
                        MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE,
                                                              TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, wk->heapId);
                        StrBuf *name = GFL_MsgDataLoadStrbufNew(msgData, MSG_INVALID_PKM_TRAINER);
                        GFL_MsgDataFree(msgData);
                        if (PML_ItemIsMail(item)) {
                            MailData *mail = CreateMailData(wk->heapId);
                            PokeParty_GetParam(pkm, PKM_PARAM_MAIL, mail);
                            MailData_SetTrainerName(mail, GFL_StrBufGetStringPtr(name));
                            PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (uptr)mail);
                            GFL_HeapFree(mail);
                        }
                        setNicknameToNick(pkm);
                        PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (uptr)GFL_StrBufGetStringPtr(name));
                        GFL_StrBufFree(name);
                    }
                    count++;
                }
            }
            wk->unk109A = 1;
            PokemonTrade_SetState(wk, PoketradeNego_StepStartMonCheck);
        } else {
            PokemonTrade_SetState(wk, PoketradeNego_StepSendCheckResult);
        }
    } else {
        u8 error;
        if (result == 15 || status == 200) {
            return;
        }
        switch (status) {
        case 400:
            error = 0xf0;
            break;
        case 401:
            error = 0xf1;
            break;
        case 408:
            error = 0xf2;
            break;
        default:
            error = 0xff;
            break;
        }
        wk->checkResult = error;
        NHttpRap_EndRequest(wk->unk0);
        PokemonTrade_SetState(wk, PoketradeNego_StepSendCheckResult);
    }
    if (wk->unk0 != NULL) {
        NHttpRap_FreePostData(wk->unk0);
        NHttpRap_Destroy(wk->unk0);
        wk->unk0 = NULL;
        DWCRapCommon_EndSubHeap();
        DWCRap_SetErrorFunc(NULL, NULL);
    }
}

// Sends the other player's Pokémon to the server to be checked
static void PoketradeNego_StepStartMonCheck(PokemonTradeWork *wk) {
    int i;
    int count = 0;
    DWCRapCommon_SetSubHeap(13, 0x10000, wk->heapId);
    wk->unk0 = NHttpRap_Create(wk->heapId, PlayerInfo_GetProfileId(wk->myInfo), wk->param->buffer);
    DWCRap_SetErrorFunc(PoketradeNego_AbortHttpRequest, wk);
    for (i = 0; i < 3; i++) {
        if (PokemonTrade_IsMonValid((u8 *)wk->negoPkm[1][i])) {
            count++;
        }
    }
    NHttpRap_BeginPost(wk->unk0, wk->heapId, PokeParty_GetPkmRawSize() * count, 2);
    for (i = 0; i < 3; i++) {
        if (PokemonTrade_IsMonValid((u8 *)wk->negoPkm[1][i])) {
            NHttpRap_AddPostData(wk->unk0, wk->negoPkm[1][i], PokeParty_GetPkmRawSize());
        }
    }
    NHttpRap_SendValidate(wk->unk0);
    NHttpRap_StartRequest(wk->unk0);
    PokemonTrade_SetState(wk, PoketradeNego_StepPollMonCheck);
}

static void PoketradeNego_StepRetryMonCheck(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        Poketrade2D_DeleteAllBoxIcons(wk);
        GFL_BGSysSetEnabledBGsB(4);
        killBrightnessEitherEngine(4);
        wk->checkCount++;
        if (wk->checkCount >= 3) {
            PokemonTrade_ClearNetAutoExit(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return;
        }
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1f, 8);
        PokemonTrade_SetState(wk, PoketradeNego_StepStartMonCheck);
    }
}

static void PoketradeNego_StepBeginMonCheckWipe(PokemonTradeWork *wk) {
    GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
    if (wk->type == 2) {
        DWCRap_SetMic(TRUE);
    }
    PokemonTrade_SetState(wk, PoketradeNego_StepRetryMonCheck);
}

static void PoketradeNego_StepResolveFinalAgree(PokemonTradeWork *wk) {
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
    PoketradeNego_CloseCancelMenu(wk, 0, 0);
    wk->unk11EA = 0;
    wk->unk11E8[0] = 3;
    wk->unk11E8[1] = 3;
    if (wk->command[myId] == 10 && wk->command[otherId] == 10) {
        PoketradeNego_DrawOfferStatusLabel(wk, 0, 1, TRUE);
        PoketradeNego_DrawOfferStatusLabel(wk, 1, 1, TRUE);
        if (wk->type == 3) {
            PokemonTrade_SetState(wk, PoketradeNego_StepBeginMonCheckWipe);
            return;
        }
        PokemonTrade_SetState(wk, PoketradeNego_StepSendReady);
        return;
    }
    if (wk->command[myId] == 4 && wk->command[otherId] == 4) {
        wk->param->next = 3;
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
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
    wk->unk11E8[0] = 0;
    wk->unk11E8[1] = 0;
    PokemonTrade_SetState(wk, PokemonTrade_StepShowWaitMessageA);
}

static void PoketradeNego_StepSendFinalAgree(PokemonTradeWork *wk) {
    NetHandle *handle = NetHandle_GetOwn();
    if (PokemonTrade_IsNetwork(wk)) {
        u8 data = 1;
        if (GFL_NetSendCommand(handle, TRADE_NET_CMD_UNKF, 1, &data)) {
            PokemonTrade_SetState(wk, PoketradeNego_StepResolveFinalAgree);
        }
    } else {
        PokemonTrade_SetState(wk, PoketradeNego_StepResolveFinalAgree);
    }
}

static void PoketradeNego_StepSendFinalReady(PokemonTradeWork *wk) {
    NetHandle *handle = NetHandle_GetOwn();
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            u8 command = 10;
            if (GFL_NetSendCommand(handle, TRADE_NET_CMD_SELECT, 1, &command)) {
                NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x1a, 8);
                PokemonTrade_SetState(wk, PoketradeNego_StepSendFinalAgree);
            }
        } else {
            wk->command[0] = 10;
            wk->command[1] = 10;
            PokemonTrade_SetState(wk, PoketradeNego_StepSendFinalAgree);
        }
    }
}

static void PoketradeNego_StepWaitMessageThenContinue(PokemonTradeWork *wk) {
    NetHandle_GetOwn();
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        PokemonTrade_SetState(wk, PokemonTrade_StepRebuildBrowseScene);
    }
}

static void PoketradeNego_StepBeginFinalReady(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, PoketradeNego_StepSendFinalReady);
}

static void PoketradeNego_StepFinalAgreeMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        if (choice == 0) {
            PokemonTrade_SetState(wk, PoketradeNego_StepBeginFinalReady);
        } else {
            PoketradeSave_CloseMessageWindowClear(wk);
            PokemonTrade_SetState(wk, PoketradeNego_StepWaitMessageThenContinue);
        }
    }
}

static void PoketradeNego_StepOpenFinalAgreeMenu(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        u32 items[] = { 24, 25 };
        PoketradeSave_OpenTaskMenuAt(wk, items, NELEMS(items), 32, 12);
        PokemonTrade_SetState(wk, PoketradeNego_StepFinalAgreeMenuChoice);
    }
}

static void PoketradeNego_StepWaitFinalMessage(PokemonTradeWork *wk) {
    PokemonTrade_GetMyNetId();
    if (PoketradeSave_IsMessagePrintDone(wk) && PokemonTrade_TickWifiTimer(wk)) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 138, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PokemonTrade_SetState(wk, PoketradeNego_StepOpenFinalAgreeMenu);
    }
}

static void PoketradeNego_StepBeginFinalWait(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, PoketradeNego_StepWaitFinalMessage);
}

static void PoketradeNego_StepCancelMenuChoice(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        PoketradeSave_CloseTaskMenu(wk);
        PoketradeSave_CloseMessageWindowClear(wk);
        wk->menu = NULL;
        if (choice == 0) {
            PokemonTrade_SetState(wk, PoketradeNego_StepBeginFinalWait);
        } else {
            PoketradeSave_CloseMessageWindowClear(wk);
            PokemonTrade_SetState(wk, PokemonTrade_StepRebuildBrowseScene);
        }
    }
}

static void PoketradeNego_StepOpenCancelMenu(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        u32 items[] = { 24, 25 };
        PoketradeSave_OpenTaskMenuAt(wk, items, NELEMS(items), 32, 12);
        PokemonTrade_SetState(wk, PoketradeNego_StepCancelMenuChoice);
    }
}

void PoketradeNego_StepShowCancelMessage(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 136, wk->strbuf);
    PoketradeSave_OpenMessageWindowTop(wk);
    TouchBar_SetIconActive(wk->touchBar, TOUCHBAR_ICON_RETURN, FALSE);
    PokemonTrade_SetState(wk, PoketradeNego_StepOpenCancelMenu);
    PoketradeSave_SetupWindowRegsSummary(wk);
}

// Whether the Pokémon at a place of the strip is held or offered
BOOL PoketradeNego_IsMonOwnOffer(PokemonTradeWork *wk, int column, int row) {
    int i;
    int box = PokemonTrade_GetColumnBox(column, wk);
    int slot = PokemonTrade_GetColumnSlot(column, row);
    if (wk->heldSlot == slot && wk->heldBox == box) {
        return TRUE;
    }
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 3; i++) {
            if (wk->negoSlot[0][i] != -1 && slot == wk->negoSlot[0][i] && box == wk->negoBox[0][i]) {
                return TRUE;
            }
        }
    } else if (wk->selectSlot == slot && wk->selectBox == box && wk->heldSlot == -1 && wk->heldBox == -1) {
        return TRUE;
    }
    return FALSE;
}

BOOL PoketradeNego_IsMonPeerSelected(PokemonTradeWork *wk, int column, int row) {
    int box = PokemonTrade_GetColumnBox(column, wk);
    int slot = PokemonTrade_GetColumnSlot(column, row);
    if (wk->unkF94 == slot && wk->unkF98 == box) {
        return TRUE;
    }
    return FALSE;
}

static void PoketradeNego_StepWaitTouchRelease(PokemonTradeWork *wk) {
    if (!GFL_HIDIsTouchHeld()) {
        PokemonTrade_SetState(wk, PokemonTrade_StepRebuildBrowseScene);
    }
}

void PoketradeNego_StepSendOfferRemoval(PokemonTradeWork *wk) {
    int index = PoketradeNego_FindOfferIndex(wk, wk->unkF98, wk->unkF94);
    if (!PokemonTrade_IsNetwork(wk) ||
        GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_WITHDRAW, sizeof(index), &index)) {
        wk->selectSlot = -1;
        wk->selectBox = -1;
        wk->heldSlot = -1;
        wk->heldBox = -1;
        wk->unkF94 = -1;
        wk->unkF98 = -1;
        if (index != -1) {
            PoketradeNego_RemoveOffer(wk, 0, index);
            Poketrade2D_DeleteOfferIconActor(wk, 0, index);
        }
        PokemonTrade_SetState(wk, PoketradeNego_StepWaitTouchRelease);
    }
}

void PoketradeNego_InitOfferMarkers(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 4; i++) {
            Poketrade2D_CreateMarkerActor(wk, i);
        }
    }
}

void PoketradeNego_SetOfferMarkersState(PokemonTradeWork *wk, BOOL a1) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 4; i++) {
            Poketrade2D_SetMarkerActorVisible(wk, i, a1);
        }
    }
}

void PoketradeNego_UpdateOfferMarkers(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 4; i++) {
            Poketrade2D_DeleteMarkerActor(wk, i);
        }
    }
}

void PoketradeNego_PollMarkerTouch(PokemonTradeWork *wk) {
    u32 x, y;
    u8 index;
    if (PokemonTrade_IsNegoType(wk) && GFL_HIDGetTouchPressedPoint(&x, &y) && x >= 8 && x < 104 && y >= 144 &&
        y < 168) {
        index = (x - 8) / 24;
        if (index >= 4) {
            index = 3;
        }
        Poketrade2D_SetMarkerActorAnim(wk, index);
        PoketradeNego_FlashMarker(wk, index, 0);
        if (PokemonTrade_IsNetwork(wk)) {
            GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNKD, 1, &index);
        }
    }
}

void PoketradeNego_TickMarkerTimers(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 2; i++) {
            if (wk->unk11E4[i] != 0) {
                wk->unk11E4[i]--;
                if (wk->unk11E4[i] == 0) {
                    Poketrade2D_DeleteMarkerButtonPair(wk, i);
                }
            }
        }
    }
}

void PoketradeNego_FlashMarker(PokemonTradeWork *wk, u32 index, int side) {
    if (PokemonTrade_IsNegoType(wk)) {
        Poketrade2D_CreateMarkerButtonActor(wk, index, side);
        wk->unk11E4[side] = 60;
        if (!GFL_SndIsPlaying(SEQ_SE_SELECT3)) {
            GFL_SndSEPlay(SEQ_SE_SELECT3);
        }
    }
}

void PoketradeNego_ResetMarkers(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 2; i++) {
            Poketrade2D_DeleteMarkerButtonPair(wk, i);
        }
        Poketrade2D_ReleaseMarkerResources(wk);
    }
}

void PoketradeNego_ExitMarkers(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNegoType(wk)) {
        Poketrade2D_RegisterMarkerResources(wk);
    }
}

void PoketradeNego_ShowOfferSlotMon(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    Poketrade2D_SetOfferIconsVisible(wk, side, FALSE);
    if (wk->unk11E2[side] != 0) {
        PoketradeSave_DeletePartnerOfferCard(wk, side);
        Poketrade3D_DeleteMcssSlot(wk, side);
    }
    Poketrade2D_LoadMonPalette(wk, side, pkm);
    PoketradeSave_DrawPartnerOfferCard(wk, side, pkm);
    GFL_BGSysQueueScrLoad(1);
    Poketrade3D_AddPokemonMcssFlipped(wk, side, 1 - side, pkm, 1 - side, 1);
}

void PoketradeNego_TickOfferSlotTimers(PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        if (wk->unk11E2[i] != 0) {
            wk->unk11E2[i]--;
            Poketrade2D_SetOfferIconsVisible(wk, i, FALSE);
            if (wk->unk11E2[i] == 0) {
                PoketradeSave_DeletePartnerOfferCard(wk, i);
                GFL_BGSysFillScrArea(1, 0, i * 16, 0, 16, 24, 16);
                GFL_BGSysQueueScrLoad(1);
                Poketrade3D_DeleteMcssSlot(wk, i);
                Poketrade2D_SetOfferIconsVisible(wk, i, TRUE);
            }
        }
    }
}

static BOOL PoketradeNego_SendOfferMon(PokemonTradeWork *wk, PartyPkm *pkm, int command) {
    NetHandle *handle = NetHandle_GetOwn();
    if (wk->unk11FB_0) {
        return FALSE;
    }
    MI_CpuCopy8(pkm, wk->recvPkm[2], PokeParty_GetPkmRawSize());
    if (!GFL_NetSendCommandToMachine(handle, 0xff, command, PokeParty_GetPkmRawSize(), wk->recvPkm[2], 0, FALSE,
                                     TRUE)) {
        return FALSE;
    }
    wk->unk11FB_0 = 1;
    return TRUE;
}
