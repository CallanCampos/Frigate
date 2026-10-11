#include "types.h"
#include "constants/arc.h"
#include "app/ui/touchbar.h"
#include "app/pokemon_trade_local.h"
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
#include "system/wipe.h"
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

static void func_ov194_021bbe14(NetHandle *handle, u32 timing, PokemonTradeWork *wk);
static void func_ov194_021bbe30(PokemonTradeWork *wk);
static BOOL func_ov194_021bbee4(PokemonTradeWork *wk);
static int func_ov194_021bbfc0(PokemonTradeWork *wk);
static int func_ov194_021bc1cc(PokemonTradeWork *wk, int side, int slot, int box, PartyPkm *pkm);
static void func_ov194_021bc7f8(PokemonTradeWork *wk);
static void func_ov194_021bc87c(PokemonTradeWork *wk);
static void func_ov194_021bc8b8(PokemonTradeWork *wk);
static void func_ov194_021bc90c(PokemonTradeWork *wk);
static void func_ov194_021bc938(PokemonTradeWork *wk);
static void func_ov194_021bc998(PokemonTradeWork *wk);
static void func_ov194_021bca98(PokemonTradeWork *wk);
static void func_ov194_021bcb08(PokemonTradeWork *wk);
static void func_ov194_021bcb60(PokemonTradeWork *wk);
static void func_ov194_021bcb9c(PokemonTradeWork *wk);
static void func_ov194_021bcbf4(PokemonTradeWork *wk);
static void func_ov194_021bcc38(PokemonTradeWork *wk);
static void func_ov194_021bcc7c(PokemonTradeWork *wk);
static void func_ov194_021bccd0(PokemonTradeWork *wk);
static void func_ov194_021bcd1c(PokemonTradeWork *wk);
static void func_ov194_021bce98(PokemonTradeWork *wk);
static void func_ov194_021bced4(PokemonTradeWork *wk);
static void func_ov194_021bcf5c(PokemonTradeWork *wk, int index);
static BOOL func_ov194_021bcfb0(PokemonTradeWork *wk);
static void func_ov194_021bd06c(PokemonTradeWork *wk);
static void func_ov194_021bd164(PokemonTradeWork *wk);
static void func_ov194_021bd190(PokemonTradeWork *wk);
static void func_ov194_021bd2c0(PokemonTradeWork *wk);
static void func_ov194_021bd36c(PokemonTradeWork *wk);
static void func_ov194_021bd414(PokemonTradeWork *wk);
static void func_ov194_021bd454(PokemonTradeWork *wk);
static void func_ov194_021bd4b4(PokemonTradeWork *wk);
static BOOL func_ov194_021bd4fc(PokemonTradeWork *wk, int index);
static void func_ov194_021bd5c8(PokemonTradeWork *wk);
static void func_ov194_021bd668(PokemonTradeWork *wk);
static void func_ov194_021bd6c0(PokemonTradeWork *wk);
static void func_ov194_021bd6f8(PokemonTradeWork *wk);
static void func_ov194_021bd730(PokemonTradeWork *wk);
static void func_ov194_021bd790(PokemonTradeWork *wk);
static void func_ov194_021bd7cc(PokemonTradeWork *wk);
static void func_ov194_021bd8dc(PokemonTradeWork *wk);
static void func_ov194_021bd948(PokemonTradeWork *wk);
static void func_ov194_021bd97c(PokemonTradeWork *wk);
static void func_ov194_021bd9f4(PokemonTradeWork *wk);
static void func_ov194_021bda48(PokemonTradeWork *wk);
static void func_ov194_021bdb00(PokemonTradeWork *wk);
static void func_ov194_021bdb1c(PokemonTradeWork *wk);
static void func_ov194_021bdb64(PokemonTradeWork *wk);
static void func_ov194_021bdba8(PokemonTradeWork *wk);
static void func_ov194_021bdc50(PokemonTradeWork *wk);
static void func_ov194_021bdca4(void *work, int a1, int code, int error);
static void func_ov194_021bdcc4(PokemonTradeWork *wk);
static void func_ov194_021bde60(PokemonTradeWork *wk);
static void func_ov194_021bdf10(PokemonTradeWork *wk);
static void func_ov194_021bdf70(PokemonTradeWork *wk);
static void func_ov194_021bdfb8(PokemonTradeWork *wk);
static void func_ov194_021be0e0(PokemonTradeWork *wk);
static void func_ov194_021be124(PokemonTradeWork *wk);
static void func_ov194_021be18c(PokemonTradeWork *wk);
static void func_ov194_021be1ac(PokemonTradeWork *wk);
static void func_ov194_021be1e4(PokemonTradeWork *wk);
static void func_ov194_021be238(PokemonTradeWork *wk);
static void func_ov194_021be274(PokemonTradeWork *wk);
static void func_ov194_021be2b8(PokemonTradeWork *wk);
static void func_ov194_021be2f0(PokemonTradeWork *wk);
static void func_ov194_021be344(PokemonTradeWork *wk);
static void func_ov194_021be494(PokemonTradeWork *wk);
static BOOL func_ov194_021be790(PokemonTradeWork *wk, PartyPkm *pkm, int command);

// The six Pokémon on the screen, the three of each player
static TouchRect sNegoPkmRects[] = {
    { 0x18, 0x44, 0x08, 0x78 }, { 0x48, 0x74, 0x08, 0x78 }, { 0x78, 0xa4, 0x08, 0x78 }, { 0x18, 0x44, 0x88, 0xf8 },
    { 0x48, 0x74, 0x88, 0xf8 }, { 0x78, 0xa4, 0x88, 0xf8 }, { TOUCH_RECT_END },
};

static void func_ov194_021bbe14(NetHandle *handle, u32 timing, PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        func_02040624(handle, timing, 8);
    }
}

static void func_ov194_021bbe30(PokemonTradeWork *wk) {
    if (func_0203d554()) {
        func_ov194_021c56f8(wk, -1);
    } else if (wk->cursor != -1) {
        func_ov194_021c56f8(wk, wk->cursor);
    }
}

BOOL func_ov194_021bbe60(PokemonTradeWork *wk, BoxPkm *pkm) {
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

static BOOL func_ov194_021bbee4(PokemonTradeWork *wk) {
    if (wk->waitTimer != 0) {
        if (func_ov194_021b783c(wk)) {
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

void func_ov194_021bbf18(PokemonTradeWork *wk) {
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

void func_ov194_021bbf5c(PokemonTradeWork *wk) {
    int side, i;
    for (side = 0; side < 2; side++) {
        for (i = 0; i < 3; i++) {
            wk->negoSlot[side][i] = -1;
            wk->negoBox[side][i] = -1;
            if (wk->negoPkm[side][i] != NULL) {
                sys_memset(wk->negoPkm[side][i], 0, PokeParty_GetPkmRawSize());
            }
        }
    }
}

// The number of Pokémon this player offers
static int func_ov194_021bbfc0(PokemonTradeWork *wk) {
    int i;
    int count = 0;
    for (i = 0; i < 3; i++) {
        if (wk->negoSlot[0][i] != -1 && wk->negoBox[0][i] != -1) {
            count++;
        }
    }
    return count;
}

BOOL func_ov194_021bbff0(PokemonTradeWork *wk) {
    if (wk->type == 3) {
        if (wk->menuWin == NULL) {
            func_ov194_021c1788(wk, 155);
        }
        if (func_ov194_021c1884(wk)) {
            func_ov194_021b772c(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return TRUE;
        }
    }
    return FALSE;
}

void func_ov194_021bc038(PokemonTradeWork *wk, u32 a1, u32 a2) {
    if (wk->type == 3) {
        func_ov194_021c1820(wk, a1, a2);
    }
}

BOOL func_ov194_021bc04c(PokemonTradeWork *wk) {
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(wk->gameData);
    int offered = func_ov194_021bbfc0(wk);
    int count = PokeParty_GetPkmCount(GameData_GetParty(wk->gameData)) + howManyTotalPokesAreInBoxes(boxes);
    if (count == 2 && offered >= 1) {
        return TRUE;
    }
    if (count > 2 && offered >= 2) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov194_021bc098(PokemonTradeWork *wk) {
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

int func_ov194_021bc0f0(PokemonTradeWork *wk, int box, int slot) {
    int i;
    for (i = 0; i < 3; i++) {
        if (slot == wk->negoSlot[0][i] && box == wk->negoBox[0][i]) {
            return i;
        }
    }
    return -1;
}

void func_ov194_021bc124(PokemonTradeWork *wk, int index, PartyPkm *pkm) {
    wk->negoSlot[1][index] = index;
    wk->negoBox[1][index] = index;
    if (wk->negoPkm[1][index] == NULL) {
        wk->negoPkm[1][index] =
            GFL_HeapAllocate(wk->heapId, PokeParty_GetPkmRawSize(), TRUE, "pokemontrade_nego.c", 293);
    }
    sys_memcpy(pkm, wk->negoPkm[1][index], PokeParty_GetPkmRawSize());
}

// The free place among a side's offers for a Pokémon, or -1 if it is offered already
int func_ov194_021bc178(PokemonTradeWork *wk, int side, int slot, int box) {
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

static int func_ov194_021bc1cc(PokemonTradeWork *wk, int side, int slot, int box, PartyPkm *pkm) {
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
            sys_memcpy(pkm, wk->negoPkm[side][i], PokeParty_GetPkmRawSize());
            func_ov194_021be6c0(wk, side, pkm);
            return i;
        }
    }
    return -1;
}

void func_ov194_021bc29c(PokemonTradeWork *wk, int side, int index) {
    wk->negoSlot[side][index] = -1;
    wk->negoBox[side][index] = -1;
    if (wk->negoPkm[side][index] != NULL) {
        GFL_HeapFree(wk->negoPkm[side][index]);
    }
    wk->negoPkm[side][index] = NULL;
}

void func_ov194_021bc2d0(PokemonTradeWork *wk, u32 vram) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(7, wk->heapId);
    u16 offset = vram == 1 ? 0x60 : 0;
    wk->objRes[TRADE_OBJRES_PLTT_NEGO] = func_0204bc48(arc, func_02021114(), vram, offset, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_NEGO] = func_0204bde0(arc, func_02021154(), getOBJTileMapping_MainEng(), wk->heapId);
    GFL_ArcToolFree(arc);
}

void func_ov194_021bc330(PokemonTradeWork *wk) {
    if (wk->objRes[TRADE_OBJRES_PLTT_NEGO] != 0) {
        func_0204bcd0(wk->objRes[TRADE_OBJRES_PLTT_NEGO]);
        wk->objRes[TRADE_OBJRES_PLTT_NEGO] = 0;
    }
    if (wk->objRes[TRADE_OBJRES_CELL_NEGO] != 0) {
        func_0204be64(wk->objRes[TRADE_OBJRES_CELL_NEGO]);
        wk->objRes[TRADE_OBJRES_CELL_NEGO] = 0;
    }
}

BOOL func_ov194_021bc35c(PokemonTradeWork *wk, int slot, int box) {
    PartyPkm *pkm;
    int index;
    func_02040440();
    pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->selectBox, wk->selectSlot, wk);
    if (PokemonTrade_IsNetwork(wk)) {
        u32 i = 0;
        index = func_ov194_021bc1cc(wk, 0, slot, box, pkm);
        if (index != -1) {
            if (!func_ov194_021be790(wk, wk->negoPkm[0][index], TRADE_NET_CMD_OFFER + index)) {
                func_ov194_021bc29c(wk, 0, index);
                GFL_HeapFree(pkm);
                return FALSE;
            }
            // The data was printed here
            for (i = 0; i < PokeParty_GetPkmRawSize(); i++) {
            }
        }
    } else {
        index = func_ov194_021bc1cc(wk, 0, slot, box, pkm);
    }
    if (index == -1) {
        GFL_HeapFree(pkm);
        return TRUE;
    }
    func_ov194_021c5138(wk, 0, index, pkm, 1, 0);
    GFL_HeapFree(pkm);
    return TRUE;
}

void func_ov194_021bc434(PokemonTradeWork *wk) {
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

void func_ov194_021bc6b4(PokemonTradeWork *wk, int side, u32 msg, BOOL force) {
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

void func_ov194_021bc784(PokemonTradeWork *wk) {
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

static void func_ov194_021bc7f8(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            GFL_WipeSet(3, 0, 0, 0, 6, 1, wk->heapId);
            PokemonTrade_SetState(wk, func_ov194_021bd2c0);
            break;
        case 1:
            func_ov194_021b76e0(wk);
            func_ov194_021bd97c(wk);
            func_ov194_021bbe30(wk);
            PokemonTrade_SetState(wk, func_ov194_021bd7cc);
            break;
        }
    }
}

static void func_ov194_021bc87c(PokemonTradeWork *wk) {
    u32 items[] = { 3, 5 };
    func_ov194_021c0214(wk, items, NELEMS(items));
    TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
    PokemonTrade_SetState(wk, func_ov194_021bc7f8);
}

static void func_ov194_021bc8b8(PokemonTradeWork *wk) {
    func_02040440();
    if (PokemonTrade_IsNetwork(wk) && !func_02040664(func_02040440(), 0x19, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bfe9c(wk);
    func_ov194_021bc038(wk, 0, 0);
    func_ov194_021b76e0(wk);
    func_ov194_021bbe30(wk);
    PokemonTrade_SetState(wk, func_ov194_021bd7cc);
}

static void func_ov194_021bc90c(PokemonTradeWork *wk) {
    func_02040440();
    if (func_ov194_021c00b0(wk)) {
        func_ov194_021bbe14(func_02040440(), 0x19, wk);
        PokemonTrade_SetState(wk, func_ov194_021bc8b8);
    }
}

static void func_ov194_021bc938(PokemonTradeWork *wk) {
    func_02040440();
    if (func_ov194_021c00b0(wk) && (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
        func_ov194_021bfe28(wk);
        func_ov194_021bfe34(wk);
        PokemonTrade_SetState(wk, func_ov194_021bc90c);
    }
}

static void func_ov194_021bc998(PokemonTradeWork *wk) {
    int msg;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    int mine, other;
    if (!func_ov194_021c00b0(wk) || !func_ov194_021bbee4(wk)) {
        return;
    }
    if (PokemonTrade_IsNetwork(wk) && !func_02040664(func_02040440(), 0x1d, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 0, 0);
    if (wk->command[myId] == 13 && wk->command[otherId] == 13) {
        func_ov194_021bbe14(func_02040440(), 13, wk);
        PokemonTrade_SetState(wk, func_ov194_021b94c0);
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
    func_ov194_021bfe28(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    PokemonTrade_SetState(wk, func_ov194_021bc938);
}

static void func_ov194_021bca98(PokemonTradeWork *wk) {
    u8 command = 13;
    if (func_ov194_021b8234(wk)) {
        command = 5;
    } else if (func_ov194_021b8330(wk)) {
        command = 7;
    } else if (func_ov194_021b8350(wk)) {
        command = 8;
    } else if (func_ov194_021b8370(wk)) {
        command = 6;
    }
    if (func_02042be8(func_02040440(), TRADE_NET_CMD_SELECT, 1, &command)) {
        func_ov194_021bbe14(func_02040440(), 0x1d, wk);
        PokemonTrade_SetState(wk, func_ov194_021bc998);
    }
}

static void func_ov194_021bcb08(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk)) {
        wk->command[0] = 13;
        wk->command[1] = 13;
        PokemonTrade_SetState(wk, func_ov194_021bc998);
        return;
    }
    if (!func_02040664(func_02040440(), 0x1e, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 0, 0);
    PokemonTrade_SetState(wk, func_ov194_021bca98);
}

static void func_ov194_021bcb60(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSelect(14)) {
        func_ov194_021bbe14(func_02040440(), 0x1d, wk);
        PokemonTrade_SetState(wk, func_ov194_021bc998);
    }
}

static void func_ov194_021bcb9c(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk)) {
        wk->command[0] = 14;
        wk->command[1] = 14;
        PokemonTrade_SetState(wk, func_ov194_021bc998);
        return;
    }
    if (!func_02040664(func_02040440(), 0x1e, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 0, 0);
    PokemonTrade_SetState(wk, func_ov194_021bcb60);
}

static void func_ov194_021bcbf4(PokemonTradeWork *wk) {
    func_ov194_021bbe14(func_02040440(), 0x1e, wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, func_ov194_021bcb08);
}

static void func_ov194_021bcc38(PokemonTradeWork *wk) {
    func_ov194_021bbe14(func_02040440(), 0x1e, wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, func_ov194_021bcb9c);
}

static void func_ov194_021bcc7c(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, func_ov194_021bcbf4);
            break;
        case 1:
            PokemonTrade_SetState(wk, func_ov194_021bcc38);
            break;
        }
    }
}

static void func_ov194_021bccd0(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        u32 items[] = { 4, 5 };
        func_ov194_021c0214(wk, items, NELEMS(items));
        GFL_BGSysSetEnabledBGsA(0x1f);
        TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
        PokemonTrade_SetState(wk, func_ov194_021bcc7c);
    }
}

static void func_ov194_021bcd1c(PokemonTradeWork *wk) {
    int msg;
    int i;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    if (!func_ov194_021c00b0(wk) || !func_ov194_021bbee4(wk)) {
        return;
    }
    if (PokemonTrade_IsNetwork(wk) && !func_02040664(func_02040440(), 0x1c, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 0, 0);
    if (wk->command[myId] == 11 && wk->command[otherId] == 11) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 147, wk->strbufTemplate);
        for (i = 0; i < 2; i++) {
            loadPokemonNicknameToStrbuf(wk->wordSet, i, PokemonTrade_GetPkm(wk, i));
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
        func_ov194_021bfe28(wk);
        PokemonTrade_SetState(wk, func_ov194_021bccd0);
        return;
    }
    if (wk->command[otherId] == 12) {
        wk->waitTimer = 120;
        msg = 148;
        if (wk->type == 3 && wk->checkCount >= 3) {
            func_ov194_021b772c(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return;
        }
    } else if (wk->command[myId] == 12) {
        wk->waitTimer = 120;
        msg = 150;
        if (wk->type == 3 && wk->checkCount >= 3) {
            func_ov194_021b772c(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return;
        }
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    func_ov194_021bfe28(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    if (PokemonTrade_IsNetwork(wk)) {
        func_02040624(func_02040440(), 0x19, 8);
    }
    wk->timer = 0;
    PokemonTrade_SetState(wk, func_ov194_021bd668);
}

static void func_ov194_021bce98(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSelect(11)) {
        func_02040624(func_02040440(), 0x1c, 8);
        PokemonTrade_SetState(wk, func_ov194_021bcd1c);
    }
}

static void func_ov194_021bced4(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    wk->waitTimer = 120;
    if (!PokemonTrade_IsNetwork(wk)) {
        PartyPkm *pkm;
        u32 size;
        wk->command[0] = 11;
        wk->command[1] = 11;
        pkm = wk->negoPkm[0][0];
        func_ov194_021b81c8(wk, 1, pkm);
        size = PokeParty_GetPkmRawSize();
        sys_memcpy(pkm, PokemonTrade_GetPkm(wk, 1), size);
        PokemonTrade_SetState(wk, func_ov194_021bcd1c);
    } else {
        PokemonTrade_SetState(wk, func_ov194_021bce98);
    }
}

// Shows the other player's Pokémon across from the one chosen
static void func_ov194_021bcf5c(PokemonTradeWork *wk, int index) {
    int slot = index % 3;
    int side = index / 3;
    int other = 1 - side;
    PartyPkm *pkm = wk->negoPkm[other][slot];
    if (func_ov194_021b774c((u8 *)pkm)) {
        func_ov194_021c0fa0(wk, pkm, other, 1);
        func_ov194_021c5244(wk, side, slot);
    }
}

static BOOL func_ov194_021bcfb0(PokemonTradeWork *wk) {
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

static void func_ov194_021bd06c(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        ArcTool *arc;
        GFL_BGSysSetBGEnabled(3, FALSE);
        func_ov194_021c5244(wk, -1, -1);
        func_ov194_021c0aec(wk, 0);
        arc = GFL_ArcSysCreateFileHandle(103, wk->heapId);
        // The library takes the palette offset as a u16, which the prototype doesn't say
        GFL_G2DIOLoadNSCRSync(arc, 11, 2, 0, (u16)wk->bg2Chars, 0, FALSE, wk->heapId);
        GFL_ArcToolFree(arc);
        func_ov194_021c200c(wk, 0);
        GFL_BGSysQueueScrLoad(3);
        wk->unk11F3 = 0;
        if (func_ov194_021b774c((u8 *)PokemonTrade_GetPkm(wk, 1))) {
            func_ov194_021b81c8(wk, 1, PokemonTrade_GetPkm(wk, 1));
        }
        if (func_ov194_021b774c((u8 *)PokemonTrade_GetPkm(wk, 0))) {
            func_ov194_021b81c8(wk, 0, PokemonTrade_GetPkm(wk, 0));
        }
        func_ov194_021bd97c(wk);
        func_ov194_021bbe30(wk);
        PokemonTrade_SetState(wk, func_ov194_021bd7cc);
        func_ov194_021b76e0(wk);
        GFL_WipeSet(3, 1, 1, 0, 6, 1, wk->heapId);
    }
}

static void func_ov194_021bd164(PokemonTradeWork *wk) {
    int wait = 3;
    if (func_02042b20()) {
        wait = 10;
    }
    if (wk->timer > wait) {
        PokemonTrade_SetState(wk, func_ov194_021bd190);
    }
}

static void func_ov194_021bd190(PokemonTradeWork *wk) {
    int index;
    GFL_BGSysSetBGEnabled(3, TRUE);
    if (!GFL_WipeIsFinished()) {
        return;
    }
    TouchBar_Main(wk->touchBar);
    if (TouchBar_GetDecided(wk->touchBar) == 1) {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, func_ov194_021bd06c);
        return;
    }
    if (TouchBar_GetTouched(wk->touchBar) != -1) {
        return;
    }
    index = func_0203da0c(sNegoPkmRects);
    if (index != TOUCH_RECT_NONE) {
        func_0203d564(TRUE);
        wk->cursor = index;
        func_ov194_021c56f8(wk, -1);
        func_ov194_021bcf5c(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021bd164);
        return;
    }
    if (func_0203d554() && PokemonTrade_GetPressedKeys()) {
        func_0203d564(FALSE);
        func_ov194_021c56f8(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021bd164);
        return;
    }
    if (func_ov194_021bcfb0(wk)) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov194_021c56f8(wk, wk->cursor);
        func_ov194_021bcf5c(wk, wk->cursor);
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021bd164);
    }
}

static void func_ov194_021bd2c0(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        u16 index = wk->cursor;
        int slot = index % 3;
        int side = index / 3;
        PartyPkm *pkm = wk->negoPkm[1 - side][slot];
        wk->unk11F3 = 1;
        if (pkm != NULL && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            func_ov194_021c0b6c(wk, pkm);
            func_ov194_021c5244(wk, side, slot);
        }
        func_ov194_021bd97c(wk);
        GFL_WipeSet(3, 1, 1, 0, 6, 1, wk->heapId);
        func_ov194_021b76e0(wk);
        func_ov194_021bbe30(wk);
        PokemonTrade_SetState(wk, func_ov194_021bd190);
    }
}

static void func_ov194_021bd36c(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, func_ov194_021bced4);
            break;
        case 1:
            GFL_WipeSet(3, 0, 0, 0, 6, 1, wk->heapId);
            TouchBar_SetIconActive(wk->touchBar, 1, FALSE);
            PokemonTrade_SetState(wk, func_ov194_021bd2c0);
            break;
        case 2:
            func_ov194_021bd97c(wk);
            func_ov194_021b76e0(wk);
            func_ov194_021bbe30(wk);
            PokemonTrade_SetState(wk, func_ov194_021bd7cc);
            break;
        }
    }
}

static void func_ov194_021bd414(PokemonTradeWork *wk) {
    u32 items[] = { 4, 3, 5 };
    func_ov194_021c0214(wk, items, NELEMS(items));
    TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
    PokemonTrade_SetState(wk, func_ov194_021bd36c);
}

static void func_ov194_021bd454(PokemonTradeWork *wk) {
    if (wk->unk11FB_0 != 1) {
        if (!PokemonTrade_IsNetwork(wk) ||
            func_02042be8(func_02040440(), TRADE_NET_CMD_UNK1, PokeParty_GetPkmRawSize(), wk->recvPkm[2])) {
            wk->unk11FB_0 = 1;
            PokemonTrade_SetState(wk, func_ov194_021bd414);
        }
    }
}

static void func_ov194_021bd4b4(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        u8 inParty = func_ov194_021b7c80(wk, 0);
        // BUG: The command's receiver reads one byte, but a Pokémon's size is sent from the one-byte local
#ifdef BUGFIX
        if (!func_02042be8(func_02040440(), TRADE_NET_CMD_UNK16, sizeof(inParty), &inParty)) {
#else
        if (!func_02042be8(func_02040440(), TRADE_NET_CMD_UNK16, PokeParty_GetPkmRawSize(), &inParty)) {
#endif
            return;
        }
    }
    PokemonTrade_SetState(wk, func_ov194_021bd454);
}

// Chooses one of the six Pokémon, returning whether it is there
static BOOL func_ov194_021bd4fc(PokemonTradeWork *wk, int index) {
    int side = index / 3;
    int slot = index % 3;
    if (index != -1) {
        if (wk->negoSlot[1 - side][slot] != -1) {
            PartyPkm *pkm = wk->negoPkm[1 - side][slot];
            u32 value = wk->unkFF0[1 - side][slot];
            if (pkm != NULL && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                wk->cursor = index;
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
                if (side != 0) {
                    PokemonTrade_SetState(wk, func_ov194_021bc87c);
                } else {
                    func_ov194_021b81c8(wk, side, pkm);
                    sys_memcpy(pkm, wk->recvPkm[2], PokeParty_GetPkmRawSize());
                    func_ov194_021b7cc0(wk, 0, value);
                    PokemonTrade_SetState(wk, func_ov194_021bd4b4);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void func_ov194_021bd5c8(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        func_ov194_021bb4b4(wk);
        func_ov194_021c45ec(0);
        func_ov194_021c4484(wk);
        wk->clactUnit = func_0204bf1c(340, 0, wk->heapId);
        func_ov194_021bfedc(wk);
        func_ov194_021c46a4(wk);
        func_ov194_021c3480(wk);
        func_ov194_021c2c84(wk);
        func_ov194_021c2de8(wk);
        func_ov194_021c2d78(wk);
        func_ov194_021c200c(wk, 0);
        GFL_BGSysSetEnabledBGsA(0x1f);
        GFL_BGSysSetEnabledBGsB(0x1e);
        func_ov194_021c2a24(wk);
        func_02042ba8(FALSE, wk->heapId);
        GXS_SetVisibleWnd(GX_WNDMASK_NONE);
        PokemonTrade_SetState(wk, func_ov194_021b9a38);
    }
}

static void func_ov194_021bd668(PokemonTradeWork *wk) {
    func_02040440();
    if (func_ov194_021c00b0(wk) && func_ov194_021bbee4(wk) && wk->timer >= 120) {
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, func_ov194_021bd5c8);
    }
}

static void func_ov194_021bd6c0(PokemonTradeWork *wk) {
    if (PokemonTrade_SendSelect(12)) {
        func_02040624(func_02040440(), 0x1c, 8);
        PokemonTrade_SetState(wk, func_ov194_021bcd1c);
    }
}

static void func_ov194_021bd6f8(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, func_ov194_021bd6c0);
}

static void func_ov194_021bd730(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        if (choice == 0) {
            PokemonTrade_SetState(wk, func_ov194_021bd6f8);
        } else {
            func_ov194_021bfe9c(wk);
            func_ov194_021b76e0(wk);
            func_ov194_021bbe30(wk);
            PokemonTrade_SetState(wk, func_ov194_021bd7cc);
        }
    }
}

static void func_ov194_021bd790(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        u32 items[] = { 24, 25 };
        func_ov194_021c0120(wk, items, NELEMS(items), 32, 12);
        PokemonTrade_SetState(wk, func_ov194_021bd730);
    }
}

static void func_ov194_021bd7cc(PokemonTradeWork *wk) {
    int index;
    TouchBar_Main(wk->touchBar);
    if (TouchBar_GetDecided(wk->touchBar) == 1) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 149, wk->strbuf);
        func_ov194_021bfe28(wk);
        TouchBar_SetIconActive(wk->touchBar, 1, FALSE);
        func_ov194_021bbe30(wk);
        PokemonTrade_SetState(wk, func_ov194_021bd790);
        return;
    }
    if (TouchBar_GetTouched(wk->touchBar) != -1) {
        return;
    }
    index = func_0203da0c(sNegoPkmRects);
    if (index != TOUCH_RECT_NONE) {
        func_0203d564(TRUE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov194_021c56f8(wk, -1);
        if (func_ov194_021bd4fc(wk, index)) {
            return;
        }
    }
    if (func_0203d554() && PokemonTrade_GetPressedKeys()) {
        wk->cursor %= 6;
        if (wk->cursor < 0) {
            wk->cursor = 0;
        }
        func_ov194_021c56f8(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_BUTTON_A && func_ov194_021bd4fc(wk, wk->cursor)) {
        return;
    }
    if (func_ov194_021bcfb0(wk)) {
        func_ov194_021c56f8(wk, wk->cursor);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
}

static void func_ov194_021bd8dc(PokemonTradeWork *wk) {
    if (func_ov194_021bbee4(wk) && func_ov194_021c00b0(wk) &&
        (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        if (func_0203d554()) {
            func_ov194_021c56f8(wk, -1);
        } else {
            func_ov194_021c56f8(wk, 0);
        }
        func_ov194_021bfe9c(wk);
        func_ov194_021b76e0(wk);
        PokemonTrade_SetState(wk, func_ov194_021bd7cc);
    }
}

static void func_ov194_021bd948(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 146, wk->strbuf);
    func_ov194_021bfe28(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, func_ov194_021bd8dc);
}

// Shows the upper screen's window over the other player's side
static void func_ov194_021bd97c(PokemonTradeWork *wk) {
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0x1b, -8);
    G2S_SetWnd0InsidePlane(0x1f, TRUE);
    G2S_SetWnd0Position(128, 0, 255, 169);
    G2S_SetWndOutsidePlane(0x1f, FALSE);
    GXS_SetVisibleWnd(GX_WNDMASK_W0);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
}

static void func_ov194_021bd9f4(PokemonTradeWork *wk) {
    if (func_ov194_021c57c4(wk)) {
        if (func_0203d554()) {
            func_ov194_021c56f8(wk, -1);
            wk->cursor = 0;
        } else {
            func_ov194_021c56f8(wk, 0);
            wk->cursor = 0;
        }
        if (GFL_WipeIsFinished()) {
            func_ov194_021bd97c(wk);
            PokemonTrade_SetState(wk, func_ov194_021bd948);
        }
    }
}

static void func_ov194_021bda48(PokemonTradeWork *wk) {
    func_ov194_021c2e04(wk);
    func_ov194_021be578(wk);
    func_ov194_021be688(wk);
    func_ov194_021c52bc(wk);
    func_ov194_021bc330(wk);
    func_ov194_021c3374(wk);
    func_ov194_021bc784(wk);
    func_ov194_021c368c(wk);
    func_ov194_021bc2d0(wk, 1);
    GFL_BGSysFillScrAsync(6, 0);
    func_0204c124(wk->actors[2], FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 1, TRUE);
    TouchBar_SetIconActive(wk->touchBar, 1, FALSE);
    func_ov194_021c12ec(wk, 6);
    wk->unk108C = 0;
    wk->unk1084 = 1;
    func_ov194_021c5994(wk);
    GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, func_ov194_021bd9f4);
}

static void func_ov194_021bdb00(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        PokemonTrade_SetState(wk, func_ov194_021bda48);
    }
}

static void func_ov194_021bdb1c(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || wk->unk1060 != 0) {
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, func_ov194_021bdb00);
    }
}

static void func_ov194_021bdb64(PokemonTradeWork *wk) {
    NetHandle *handle = func_02040440();
    if (PokemonTrade_IsNetwork(wk)) {
        u8 data = 1;
        if (func_02042be8(handle, TRADE_NET_CMD_UNKA, 1, &data)) {
            PokemonTrade_SetState(wk, func_ov194_021bdb1c);
        }
    } else {
        PokemonTrade_SetState(wk, func_ov194_021bdb1c);
    }
}

static void func_ov194_021bdba8(PokemonTradeWork *wk) {
    if (func_02040664(func_02040440(), 0x15, 8)) {
        int msg;
        u8 result = wk->checkResult;
        if (result == 0 && wk->partnerCheckResult == 0) {
            PokemonTrade_SetState(wk, func_ov194_021bdb64);
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
        func_ov194_021bfe28(wk);
        wk->waitTimer = 120;
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021bd668);
    }
}

static void func_ov194_021bdc50(PokemonTradeWork *wk) {
    NetHandle *handle = func_02040440();
    wk->unk109A = 0;
    if (func_02042be8(handle, TRADE_NET_CMD_CHECK_RESULT, 1, &wk->checkResult)) {
        func_02040624(func_02040440(), 0x15, 8);
        if (wk->type == 2) {
            DWCRap_SetMic(FALSE);
        }
        PokemonTrade_SetState(wk, func_ov194_021bdba8);
    }
}

static void func_ov194_021bdca4(void *work, int a1, int code, int error) {
    PokemonTradeWork *wk = work;
    if (wk->unk0 != NULL) {
        NHttpRap_EndRequest(wk->unk0);
        NHttpRap_Destroy(wk->unk0);
        wk->unk0 = NULL;
        DWCRapCommon_EndSubHeap();
    }
}

// Waits for the server's check of the other player's Pokémon
static void func_ov194_021bdcc4(PokemonTradeWork *wk) {
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
                if (func_ov194_021b774c((u8 *)wk->negoPkm[1][i])) {
                    PartyPkm *pkm = wk->negoPkm[1][i];
                    if (NHttpRap_GetCheckResult(response, count)) {
                        u32 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
                        MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, wk->heapId);
                        StrBuf *name = GFL_MsgDataLoadStrbufNew(msgData, MSG_INVALID_PKM_TRAINER);
                        GFL_MsgDataFree(msgData);
                        if (PML_ItemIsMail(item)) {
                            MailData *mail = CreateMailData(wk->heapId);
                            PokeParty_GetParam(pkm, PKM_PARAM_MAIL, mail);
                            func_02009738(mail, GFL_StrBufGetStringPtr(name));
                            PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (u32)mail);
                            GFL_HeapFree(mail);
                        }
                        setNicknameToNick(pkm);
                        PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (u32)GFL_StrBufGetStringPtr(name));
                        GFL_StrBufFree(name);
                    }
                    count++;
                }
            }
            wk->unk109A = 1;
            PokemonTrade_SetState(wk, func_ov194_021bde60);
        } else {
            PokemonTrade_SetState(wk, func_ov194_021bdc50);
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
        PokemonTrade_SetState(wk, func_ov194_021bdc50);
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
static void func_ov194_021bde60(PokemonTradeWork *wk) {
    int i;
    int count = 0;
    DWCRapCommon_SetSubHeap(13, 0x10000, wk->heapId);
    wk->unk0 = NHttpRap_Create(wk->heapId, func_02008bdc(wk->myInfo), wk->param->buffer);
    DWCRap_SetErrorFunc(func_ov194_021bdca4, wk);
    for (i = 0; i < 3; i++) {
        if (func_ov194_021b774c((u8 *)wk->negoPkm[1][i])) {
            count++;
        }
    }
    NHttpRap_BeginPost(wk->unk0, wk->heapId, PokeParty_GetPkmRawSize() * count, 2);
    for (i = 0; i < 3; i++) {
        if (func_ov194_021b774c((u8 *)wk->negoPkm[1][i])) {
            NHttpRap_AddPostData(wk->unk0, wk->negoPkm[1][i], PokeParty_GetPkmRawSize());
        }
    }
    NHttpRap_SendValidate(wk->unk0);
    NHttpRap_StartRequest(wk->unk0);
    PokemonTrade_SetState(wk, func_ov194_021bdcc4);
}

static void func_ov194_021bdf10(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        func_ov194_021c368c(wk);
        GFL_BGSysSetEnabledBGsB(4);
        killBrightnessEitherEngine(4);
        wk->checkCount++;
        if (wk->checkCount >= 3) {
            func_ov194_021b772c(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
            return;
        }
        func_02040624(func_02040440(), 0x1f, 8);
        PokemonTrade_SetState(wk, func_ov194_021bde60);
    }
}

static void func_ov194_021bdf70(PokemonTradeWork *wk) {
    GFL_WipeSet(4, 0, 0, 0, 6, 1, wk->heapId);
    if (wk->type == 2) {
        DWCRap_SetMic(TRUE);
    }
    PokemonTrade_SetState(wk, func_ov194_021bdf10);
}

static void func_ov194_021bdfb8(PokemonTradeWork *wk) {
    int msg;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    if (!PokemonTrade_IsNetwork(wk)) {
        myId = 0;
        otherId = 1;
    }
    if (PokemonTrade_IsNetwork(wk) && !func_02040664(func_02040440(), 0x1a, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 0, 0);
    wk->unk11EA = 0;
    wk->unk11E8[0] = 3;
    wk->unk11E8[1] = 3;
    if (wk->command[myId] == 10 && wk->command[otherId] == 10) {
        func_ov194_021bc6b4(wk, 0, 1, TRUE);
        func_ov194_021bc6b4(wk, 1, 1, TRUE);
        if (wk->type == 3) {
            PokemonTrade_SetState(wk, func_ov194_021bdf70);
            return;
        }
        PokemonTrade_SetState(wk, func_ov194_021bdb64);
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
    func_ov194_021bfe28(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    wk->unk11E8[0] = 0;
    wk->unk11E8[1] = 0;
    PokemonTrade_SetState(wk, func_ov194_021b9c44);
}

static void func_ov194_021be0e0(PokemonTradeWork *wk) {
    NetHandle *handle = func_02040440();
    if (PokemonTrade_IsNetwork(wk)) {
        u8 data = 1;
        if (func_02042be8(handle, TRADE_NET_CMD_UNKF, 1, &data)) {
            PokemonTrade_SetState(wk, func_ov194_021bdfb8);
        }
    } else {
        PokemonTrade_SetState(wk, func_ov194_021bdfb8);
    }
}

static void func_ov194_021be124(PokemonTradeWork *wk) {
    NetHandle *handle = func_02040440();
    if (func_ov194_021c00b0(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            u8 command = 10;
            if (func_02042be8(handle, TRADE_NET_CMD_SELECT, 1, &command)) {
                func_02040624(func_02040440(), 0x1a, 8);
                PokemonTrade_SetState(wk, func_ov194_021be0e0);
            }
        } else {
            wk->command[0] = 10;
            wk->command[1] = 10;
            PokemonTrade_SetState(wk, func_ov194_021be0e0);
        }
    }
}

static void func_ov194_021be18c(PokemonTradeWork *wk) {
    func_02040440();
    if (func_ov194_021c00b0(wk)) {
        PokemonTrade_SetState(wk, func_ov194_021ba924);
    }
}

static void func_ov194_021be1ac(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, func_ov194_021be124);
}

static void func_ov194_021be1e4(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        if (choice == 0) {
            PokemonTrade_SetState(wk, func_ov194_021be1ac);
        } else {
            func_ov194_021bfe9c(wk);
            PokemonTrade_SetState(wk, func_ov194_021be18c);
        }
    }
}

static void func_ov194_021be238(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        u32 items[] = { 24, 25 };
        func_ov194_021c0120(wk, items, NELEMS(items), 32, 12);
        PokemonTrade_SetState(wk, func_ov194_021be1e4);
    }
}

static void func_ov194_021be274(PokemonTradeWork *wk) {
    PokemonTrade_GetMyNetId();
    if (func_ov194_021c00b0(wk) && func_ov194_021bbee4(wk)) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 138, wk->strbuf);
        func_ov194_021bfe28(wk);
        PokemonTrade_SetState(wk, func_ov194_021be238);
    }
}

static void func_ov194_021be2b8(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    wk->waitTimer = 120;
    PokemonTrade_SetState(wk, func_ov194_021be274);
}

static void func_ov194_021be2f0(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        if (choice == 0) {
            PokemonTrade_SetState(wk, func_ov194_021be2b8);
        } else {
            func_ov194_021bfe9c(wk);
            PokemonTrade_SetState(wk, func_ov194_021ba924);
        }
    }
}

static void func_ov194_021be344(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        u32 items[] = { 24, 25 };
        func_ov194_021c0120(wk, items, NELEMS(items), 32, 12);
        PokemonTrade_SetState(wk, func_ov194_021be2f0);
    }
}

void func_ov194_021be380(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 136, wk->strbuf);
    func_ov194_021bfe28(wk);
    TouchBar_SetIconActive(wk->touchBar, 1, FALSE);
    PokemonTrade_SetState(wk, func_ov194_021be344);
    func_ov194_021c14b0(wk);
}

// Whether the Pokémon at a place of the strip is held or offered
BOOL func_ov194_021be3bc(PokemonTradeWork *wk, int column, int row) {
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

BOOL func_ov194_021be45c(PokemonTradeWork *wk, int column, int row) {
    int box = PokemonTrade_GetColumnBox(column, wk);
    int slot = PokemonTrade_GetColumnSlot(column, row);
    if (wk->unkF94 == slot && wk->unkF98 == box) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov194_021be494(PokemonTradeWork *wk) {
    if (!func_0203da2c()) {
        PokemonTrade_SetState(wk, func_ov194_021ba924);
    }
}

void func_ov194_021be4b0(PokemonTradeWork *wk) {
    int index = func_ov194_021bc0f0(wk, wk->unkF98, wk->unkF94);
    if (!PokemonTrade_IsNetwork(wk) || func_02042be8(func_02040440(), TRADE_NET_CMD_WITHDRAW, sizeof(index), &index)) {
        wk->selectSlot = -1;
        wk->selectBox = -1;
        wk->heldSlot = -1;
        wk->heldBox = -1;
        wk->unkF94 = -1;
        wk->unkF98 = -1;
        if (index != -1) {
            func_ov194_021bc29c(wk, 0, index);
            func_ov194_021c50d8(wk, 0, index);
        }
        PokemonTrade_SetState(wk, func_ov194_021be494);
    }
}

void func_ov194_021be534(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 4; i++) {
            func_ov194_021c551c(wk, i);
        }
    }
}

void func_ov194_021be554(PokemonTradeWork *wk, BOOL a1) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 4; i++) {
            func_ov194_021c5594(wk, i, a1);
        }
    }
}

void func_ov194_021be578(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 4; i++) {
            func_ov194_021c55ac(wk, i);
        }
    }
}

void func_ov194_021be598(PokemonTradeWork *wk) {
    u32 x, y;
    u8 index;
    if (PokemonTrade_IsNegoType(wk) && func_0203dac8(&x, &y) && x >= 8 && x < 104 && y >= 144 && y < 168) {
        index = (x - 8) / 24;
        if (index >= 4) {
            index = 3;
        }
        func_ov194_021c55c8(wk, index);
        func_ov194_021be648(wk, index, 0);
        if (PokemonTrade_IsNetwork(wk)) {
            func_02042be8(func_02040440(), TRADE_NET_CMD_UNKD, 1, &index);
        }
    }
}

void func_ov194_021be610(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 2; i++) {
            if (wk->unk11E4[i] != 0) {
                wk->unk11E4[i]--;
                if (wk->unk11E4[i] == 0) {
                    func_ov194_021c56b8(wk, i);
                }
            }
        }
    }
}

void func_ov194_021be648(PokemonTradeWork *wk, u32 index, int side) {
    if (PokemonTrade_IsNegoType(wk)) {
        func_ov194_021c55e4(wk, index, side);
        wk->unk11E4[side] = 60;
        if (!GFL_SndIsPlaying(SEQ_SE_SELECT3)) {
            GFL_SndSEPlay(SEQ_SE_SELECT3);
        }
    }
}

void func_ov194_021be688(PokemonTradeWork *wk) {
    int i;
    if (PokemonTrade_IsNegoType(wk)) {
        for (i = 0; i < 2; i++) {
            func_ov194_021c56b8(wk, i);
        }
        func_ov194_021c466c(wk);
    }
}

void func_ov194_021be6ac(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNegoType(wk)) {
        func_ov194_021c4600(wk);
    }
}

void func_ov194_021be6c0(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    func_ov194_021c510c(wk, side, FALSE);
    if (wk->unk11E2[side] != 0) {
        func_ov194_021c1740(wk, side);
        func_ov194_021c24dc(wk, side);
    }
    func_ov194_021c5d10(wk, side, pkm);
    func_ov194_021c1530(wk, side, pkm);
    GFL_BGSysQueueScrLoad(1);
    func_ov194_021c24ac(wk, side, 1 - side, pkm, 1 - side, 1);
}

void func_ov194_021be720(PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        if (wk->unk11E2[i] != 0) {
            wk->unk11E2[i]--;
            func_ov194_021c510c(wk, i, FALSE);
            if (wk->unk11E2[i] == 0) {
                func_ov194_021c1740(wk, i);
                GFL_BGSysFillScrArea(1, 0, i * 16, 0, 16, 24, 16);
                GFL_BGSysQueueScrLoad(1);
                func_ov194_021c24dc(wk, i);
                func_ov194_021c510c(wk, i, TRUE);
            }
        }
    }
}

static BOOL func_ov194_021be790(PokemonTradeWork *wk, PartyPkm *pkm, int command) {
    NetHandle *handle = func_02040440();
    if (wk->unk11FB_0) {
        return FALSE;
    }
    sys_memcpy(pkm, wk->recvPkm[2], PokeParty_GetPkmRawSize());
    if (!func_02042c18(handle, 0xff, command, PokeParty_GetPkmRawSize(), wk->recvPkm[2], 0, FALSE, TRUE)) {
        return FALSE;
    }
    wk->unk11FB_0 = 1;
    return TRUE;
}
