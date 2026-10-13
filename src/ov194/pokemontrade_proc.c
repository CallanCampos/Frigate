#include "types.h"
#include "app/ui/ui_scene.h"
#include "app/ui/touchbar.h"
#include "app/pokemon_trade_local.h"
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

static BOOL func_ov194_021b7708(PokemonTradeWork *wk);
static void PokemonTrade_UpdateBGM(PokemonTradeWork *wk);
static void func_ov194_021b7898(PokemonTradeWork *wk);
static void func_ov194_021b78b4(PokemonTradeWork *wk);
static void func_ov194_021b796c(PokemonTradeWork *wk);
static void func_ov194_021b79cc(PokemonTradeWork *wk);
static void func_ov194_021b79e4(PokemonTradeWork *wk);
static void func_ov194_021b7a2c(PokemonTradeWork *wk, ClActor *icon, int a2, int a3, BoxPkm *pkm);
static void func_ov194_021b7be0(PokemonTradeWork *wk);
static void *func_ov194_021b7bf0(int netId, void *work, int size);
static void *func_ov194_021b7c0c(int netId, void *work, int size);
static void *func_ov194_021b7c28(int netId, void *work, int size);
static void func_ov194_021b7c4c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7d44(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7d98(int netId, int size, const void *data, void *work, NetHandle *handle, int index);
static void func_ov194_021b7e0c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7e20(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7e34(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7e48(u32 value, int index, int netId, PokemonTradeWork *wk, NetHandle *handle);
static void func_ov194_021b7e84(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7e9c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7eb4(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7ecc(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7ef0(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7f1c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7f38(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7f7c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b7fa8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b803c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b805c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8088(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b80bc(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b80e4(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b810c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8134(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8140(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8164(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov194_021b8170(int netId, int size, const void *data, void *work, NetHandle *handle);
static void PokemonTrade_WaitFadeOutToEnd(PokemonTradeWork *wk);
static void func_ov194_021b8200(PokemonTradeWork *wk, int side);
static BOOL func_ov194_021b82a8(PokemonTradeWork *wk, int box, int slot);
static BOOL PokemonTrade_IsBadPkm(PartyPkm *pkm);
static void func_ov194_021b83d0(PokemonTradeWork *wk);
static void func_ov194_021b84b0(PokemonTradeWork *wk);
static void func_ov194_021b84e0(PokemonTradeWork *wk, int side, BOOL a2);
static void func_ov194_021b8570(PokemonTradeWork *wk);
static void func_ov194_021b860c(PokemonTradeWork *wk);
static void func_ov194_021b8754(PokemonTradeWork *wk);
static void func_ov194_021b87b0(PokemonTradeWork *wk);
static void func_ov194_021b8838(PokemonTradeWork *wk);
static void func_ov194_021b8894(PokemonTradeWork *wk);
static void func_ov194_021b88e4(PokemonTradeWork *wk);
static void func_ov194_021b8924(PokemonTradeWork *wk);
static void func_ov194_021b8960(PokemonTradeWork *wk);
static void func_ov194_021b8a5c(PokemonTradeWork *wk);
static void func_ov194_021b8ac0(PokemonTradeWork *wk);
static void func_ov194_021b8b24(PokemonTradeWork *wk);
static void func_ov194_021b8b6c(PokemonTradeWork *wk);
static void func_ov194_021b8bb4(PokemonTradeWork *wk);
static void func_ov194_021b8c08(PokemonTradeWork *wk, u32 a1, u32 a2);
static int func_ov194_021b8c84(PokemonTradeWork *wk, BOOL a1);
static void func_ov194_021b8dc4(PokemonTradeWork *wk);
static void func_ov194_021b8e2c(PokemonTradeWork *wk, u32 msg, BOOL a2);
static void func_ov194_021b8e74(PokemonTradeWork *wk);
static void func_ov194_021b8e80(PokemonTradeWork *wk);
static void func_ov194_021b8e8c(PokemonTradeWork *wk);
static void func_ov194_021b8e98(PokemonTradeWork *wk);
static void func_ov194_021b8ef4(PokemonTradeWork *wk);
static void func_ov194_021b8f1c(PokemonTradeWork *wk);
static void func_ov194_021b8f94(PokemonTradeWork *wk);
static void func_ov194_021b8fbc(PokemonTradeWork *wk);
static void func_ov194_021b9034(PokemonTradeWork *wk);
static BOOL func_ov194_021b8c1c(PokemonTradeWork *wk, int *box, int *slot, int *a3, int *a4);
static void func_ov194_021b9058(PokemonTradeWork *wk);
static void func_ov194_021b9394(PokemonTradeWork *wk);
static void func_ov194_021b9454(PokemonTradeWork *wk);
static void func_ov194_021b94ec(PokemonTradeWork *wk);
static void func_ov194_021b95ec(PokemonTradeWork *wk);
static void func_ov194_021b964c(PokemonTradeWork *wk);
static void func_ov194_021b9744(PokemonTradeWork *wk);
static void func_ov194_021b976c(PokemonTradeWork *wk);
static void func_ov194_021b97a0(PokemonTradeWork *wk);
static void func_ov194_021b97d0(PokemonTradeWork *wk);
static void func_ov194_021b9838(PokemonTradeWork *wk);
static void func_ov194_021b9860(PokemonTradeWork *wk);
static void func_ov194_021b9888(PokemonTradeWork *wk);
static void func_ov194_021b9904(PokemonTradeWork *wk);
static void func_ov194_021b993c(PokemonTradeWork *wk);
static void func_ov194_021b9974(PokemonTradeWork *wk);
static void func_ov194_021b99b4(PokemonTradeWork *wk);
static void func_ov194_021b9ad8(PokemonTradeWork *wk);
static void func_ov194_021b9b80(PokemonTradeWork *wk);
static void func_ov194_021b9c1c(PokemonTradeWork *wk);
static void func_ov194_021b9cbc(PokemonTradeWork *wk);
static void func_ov194_021b9d0c(PokemonTradeWork *wk);
static void func_ov194_021b9d84(PokemonTradeWork *wk);
static void func_ov194_021b9e60(PokemonTradeWork *wk);
static void func_ov194_021b9eb8(PokemonTradeWork *wk);
static void func_ov194_021b9eec(PokemonTradeWork *wk);
static BOOL func_ov194_021b9f10(PokemonTradeWork *wk, u32 a1);
static void func_ov194_021b9fcc(PokemonTradeWork *wk);
static void func_ov194_021b9fe4(PokemonTradeWork *wk);
static void func_ov194_021ba03c(PokemonTradeWork *wk);
static void func_ov194_021ba058(PokemonTradeWork *wk);
static void func_ov194_021ba170(PokemonTradeWork *wk);
static void func_ov194_021ba1b0(PokemonTradeWork *wk);
static int PokemonTrade_GetColumnX(int column);
static BOOL func_ov194_021ba240(PokemonTradeWork *wk, int column);
static void func_ov194_021b9b00(PokemonTradeWork *wk);
static BOOL func_ov194_021b9df4(PokemonTradeWork *wk);
static void func_ov194_021ba280(PokemonTradeWork *wk);
static void PokemonTrade_Scroll(PokemonTradeWork *wk, BOOL playSound, BOOL send);
static void func_ov194_021ba5f0(PokemonTradeWork *wk);
static void func_ov194_021ba6ec(PokemonTradeWork *wk);
static void func_ov194_021ba708(PokemonTradeWork *wk);
static void func_ov194_021ba78c(PokemonTradeWork *wk);
static void func_ov194_021ba7d4(PokemonTradeWork *wk);
static void func_ov194_021ba84c(PokemonTradeWork *wk);
static void func_ov194_021ba8c0(PokemonTradeWork *wk);
static void func_ov194_021ba990(PokemonTradeWork *wk);
static void func_ov194_021ba9ec(PokemonTradeWork *wk);
static void func_ov194_021baa90(PokemonTradeWork *wk);
static BOOL func_ov194_021baac4(PokemonTradeWork *wk, int box, int slot);
static void func_ov194_021bb044(PokemonTradeWork *wk);
static void func_ov194_021bb104(PokemonTradeWork *wk);
static void func_ov194_021bb180(PokemonTradeWork *wk);
static void func_ov194_021bb1bc(PokemonTradeWork *wk);
static void func_ov194_021bb1ec(PokemonTradeWork *wk);
static void func_ov194_021bb218(PokemonTradeWork *wk);
static void func_ov194_021bb228(PokemonTradeWork *wk);
static void func_ov194_021bbdac(PokemonTradeWork *wk, BOOL a1);
static void PokemonTrade_VBlank(TCB *tcb, void *data);
static void func_ov194_021bb2cc(PokemonTradeWork *wk, GameData *gameData, u16 friendIndex);
static void func_ov194_021bb364(PokemonTradeWork *wk, BOOL a1);
static void func_ov194_021bb384(PokemonTradeWork *wk);
static void func_ov194_021bb3c0(PokemonTradeWork *wk, int box);
static void PokemonTrade_RestoreBGM(PokemonTradeWork *wk);
static PokemonTradeWork *PokemonTrade_CreateWork(GameProc *proc, u32 heapSize);
static BOOL PokemonTrade_Init(GameProc *proc, u32 *state, PokemonTradeParam *param, PokemonTradeWork *wk, int type);
static BOOL PokemonTrade_ProcInitWifiClub(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcInitIrc(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov194_021bb868(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcInitGtsNego(GameProc *proc, u32 *state, void *param, void *work);
static void func_ov194_021bb938(PokemonTradeDemoParam *demo, PokemonTradeWork *wk);
static BOOL func_ov194_021bb9cc(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov194_021bba0c(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov194_021bba4c(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov194_021bba8c(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokemonTrade_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov194_021bab08(PokemonTradeWork *wk);
static void func_ov194_021babc4(PokemonTradeWork *wk);

// MWCC lays the file's data out by a heapsort over the order of declaration, which is not stable: the tables below
// are declared where they are, and the proc tables at the end in their order, because that gives the original's
// `.rodata` (see `tools/decomp/rodata_order.py`)
static const TouchRect sSideButtonRects[3];

static const NetCommand sNetCommands[] = {
    { func_ov194_021b803c, NULL },
    { func_ov194_021b7d44, func_ov194_021b7bf0 },
    { func_ov194_021b8170, NULL },
    { func_ov194_021b8140, NULL },
    { func_ov194_021b8134, NULL },
    { func_ov194_021b8164, NULL },
    { func_ov194_021b80bc, NULL },
    { func_ov194_021b7e0c, func_ov194_021b7c0c },
    { func_ov194_021b7e20, func_ov194_021b7c0c },
    { func_ov194_021b7e34, func_ov194_021b7c0c },
    { func_ov194_021b805c, NULL },
    { func_ov194_021b8088, NULL },
    { func_ov194_021b80e4, NULL },
    { func_ov194_021b810c, NULL },
    { func_ov194_021b7f1c, func_ov194_021b7c28 },
    { func_ov194_021b7c4c, NULL },
    { func_ov194_021b7f38, NULL },
    { func_ov194_021b7f7c, NULL },
    { func_ov194_021b7fa8, NULL },
    { func_ov194_021b7e84, NULL },
    { func_ov194_021b7e9c, NULL },
    { func_ov194_021b7eb4, NULL },
    { func_ov194_021b7ecc, NULL },
    { func_ov194_021b7ef0, NULL },
};

// The memory of the trade's heap
static u8 sHeapMemory[0xc000];

void func_ov194_021b76e0(PokemonTradeWork *wk) {
    TouchBar_Reset(wk->touchBar, 1);
    TouchBar_SetIconVisible(wk->touchBar, 1, TRUE);
    TouchBar_SetIconActive(wk->touchBar, 1, TRUE);
}

static BOOL func_ov194_021b7708(PokemonTradeWork *wk) {
    void *save = func_02009790(wk->gameData);
    BOOL ret = FALSE;
    if (func_020097c4(save, 0) == -1) {
        ret = TRUE;
    }
    return ret;
}

void func_ov194_021b772c(PokemonTradeWork *wk) {
    func_02042e94(FALSE);
    func_02042e9c(FALSE);
    func_020421ac(FALSE);
    wk->param->next = 3;
}

BOOL func_ov194_021b774c(const u8 *data) {
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

u8 func_ov194_021b7778(PokemonTradeWork *wk) {
    return wk->partnerBoxCount;
}

BOOL PokemonTrade_IsNetwork(PokemonTradeWork *wk) {
    if (wk->param == NULL) {
        return FALSE;
    }
    if (wk->type >= 4) {
        return FALSE;
    }
    if (func_02042788()) {
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

BOOL func_ov194_021b783c(PokemonTradeWork *wk) {
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

static void func_ov194_021b7898(PokemonTradeWork *wk) {
    wk->firstColumn++;
    wk->unk1084 = 1;
    PokemonTrade_Scroll(wk, 0, 0);
}

static void func_ov194_021b78b4(PokemonTradeWork *wk) {
    u32 x, y;
    if (func_0203da84(&x, &y)) {
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

static void func_ov194_021b796c(PokemonTradeWork *wk) {
    u32 x, y;
    ClActorPos pos;
    if (wk->heldIcon != NULL && func_0203da84(&x, &y)) {
        func_0204c178(wk->heldIcon, &wk->heldPrevPos, 1);
        pos.x = x + wk->heldOffsetX;
        pos.y = y + wk->heldOffsetY;
        if (pos.y > 150) {
            pos.y = 150;
        }
        func_0204c140(wk->heldIcon, &pos, 1);
    }
}

static void func_ov194_021b79cc(PokemonTradeWork *wk) {
    if (wk->heldIcon != NULL) {
        wk->heldIcon = NULL;
    }
    func_ov194_021c54ec(wk);
}

static void func_ov194_021b79e4(PokemonTradeWork *wk) {
    if (wk->heldIcon != NULL) {
        func_0204c438(wk->heldIcon, 16);
        func_0204c140(wk->heldIcon, &wk->heldPos, 1);
        func_0204c124(wk->heldIcon, TRUE);
        wk->heldSlot = -1;
        wk->heldBox = -1;
        func_ov194_021b79cc(wk);
        sys_memset(wk->unk4, 0, sizeof(wk->unk4));
    }
}

static void func_ov194_021b7a2c(PokemonTradeWork *wk, ClActor *icon, int a2, int a3, BoxPkm *pkm) {
    u32 x, y;
    wk->heldIcon = icon;
    func_0204c178(icon, &wk->heldPos, 1);
    func_ov194_021c38bc(wk, wk->heldIcon, 1);
    if (func_0203da84(&x, &y)) {
        wk->heldOffsetX = wk->heldPos.x - x;
        wk->heldOffsetY = wk->heldPos.y - y;
        func_ov194_021c52e0(wk, a2, a3, x + wk->heldOffsetX, y + wk->heldOffsetY, pkm);
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
                sys_memcpy(PokeParty_GetPkm(party, slot), pkm, size);
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
            return func_0201d624(PokeParty_GetPkm(party, slot));
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

static void func_ov194_021b7be0(PokemonTradeWork *wk) {
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

static void *func_ov194_021b7c28(int netId, void *work, int size) {
    PokemonTradeWork *wk = work;
    if (netId == PokemonTrade_GetMyNetId()) {
        return &wk->boxColors[0];
    }
    return &wk->boxColors[1];
}

static void func_ov194_021b7c4c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440()) {
        wk->unk11E8[netId != PokemonTrade_GetMyNetId() ? 1 : 0] = *(const u8 *)data;
    }
}

u32 func_ov194_021b7c80(PokemonTradeWork *wk, int side) {
    if (!PokemonTrade_IsNetwork(wk)) {
        return wk->unk5E4[side];
    }
    if (PokemonTrade_GetMyNetId() == 0) {
        return wk->unk5E4[side];
    }
    return wk->unk5E4[1 - side];
}

void func_ov194_021b7cc0(PokemonTradeWork *wk, int side, u32 value) {
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

static void func_ov194_021b7d44(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440()) {
        u32 pkmSize = PokeParty_GetPkmRawSize();
        sys_memcpy(wk->recvPkm[netId], wk->pkm[netId], pkmSize);
        if (netId != PokemonTrade_GetMyNetId()) {
            wk->unkF9C = netId + 1;
            wk->unk11FB_4 = 1;
        }
    }
}

static void func_ov194_021b7d98(int netId, int size, const void *data, void *work, NetHandle *handle, int index) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        if (PokeParty_GetParam(wk->recvPkm[netId], PKM_PARAM_SPECIES_VALID, NULL)) {
            func_ov194_021bc124(wk, index, wk->recvPkm[netId]);
            func_ov194_021c5138(wk, 1, index, wk->recvPkm[netId], 1, 0);
            func_ov194_021be6c0(wk, 1, wk->recvPkm[netId]);
        }
        wk->unk11FB_4 = 1;
    }
}

static void func_ov194_021b7e0c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_ov194_021b7d98(netId, size, data, work, handle, 0);
}

static void func_ov194_021b7e20(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_ov194_021b7d98(netId, size, data, work, handle, 1);
}

static void func_ov194_021b7e34(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_ov194_021b7d98(netId, size, data, work, handle, 2);
}

static void func_ov194_021b7e48(u32 value, int index, int netId, PokemonTradeWork *wk, NetHandle *handle) {
    if (handle == func_02040440()) {
        if (netId == PokemonTrade_GetMyNetId()) {
            wk->unkFF0[0][index] = value;
        } else {
            wk->unkFF0[1][index] = value;
        }
    }
}

static void func_ov194_021b7e84(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_ov194_021b7e48(*(const u8 *)data, 0, netId, work, handle);
}

static void func_ov194_021b7e9c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_ov194_021b7e48(*(const u8 *)data, 1, netId, work, handle);
}

static void func_ov194_021b7eb4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_ov194_021b7e48(*(const u8 *)data, 2, netId, work, handle);
}

static void func_ov194_021b7ecc(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440()) {
        wk->unk5E4[netId] = *(const u8 *)data;
    }
}

static void func_ov194_021b7ef0(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        wk->unk11FB_0 = 0;
    }
}

static void func_ov194_021b7f1c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != func_02040440()) {
        return;
    }
    if (netId == PokemonTrade_GetMyNetId()) {
        return;
    }
}

static void func_ov194_021b7f38(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        const u8 *bytes = data;
        wk->partnerBoxCount = bytes[0];
        wk->unk11F4 = bytes[1];
        wk->unk107E = wk->partnerBoxCount * 160 + 16;
    }
}

static void func_ov194_021b7f7c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        wk->partnerCheckResult = *(const u8 *)data;
    }
}

static void func_ov194_021b7fa8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    UnityTowerVisitor profile;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(wk->gameData));
        u8 country, province, validCountry, validProvince;
        sys_memcpy(data, &profile, sizeof(UnityTowerVisitor));
        country = UnityTowerVisitor_GetCountry(&profile.info);
        province = UnityTowerVisitor_GetProvince(&profile.info);
        validCountry = Country_GetValidCountry(country, province, TrainerInfo_GetRegion(&profile.info));
        validProvince = Country_GetValidRegion(country, province, TrainerInfo_GetRegion(&profile.info));
        if (country == validCountry && province == validProvince) {
            func_02008c14(&profile.info, country, province);
        } else {
            func_02008c14(&profile.info, 0, 0);
        }
        UnityTowerSurvey_RegisterTrade(survey, &profile);
    }
}

static void func_ov194_021b803c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440()) {
        wk->command[netId] = *(const u8 *)data;
    }
}

static void func_ov194_021b805c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        wk->unk1060 = *(const u8 *)data;
    }
}

static void func_ov194_021b8088(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        func_ov194_021bc29c(wk, 1, *(const u32 *)data);
        func_ov194_021c50d8(wk, 1, *(const u32 *)data);
    }
}

static void func_ov194_021b80bc(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != PokemonTrade_GetMyNetId()) {
        func_ov194_021b8200(wk, 1);
    }
}

static void func_ov194_021b80e4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != func_0203ffc4()) {
        wk->unk107E = *(const s16 *)data;
    }
}

static void func_ov194_021b810c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != func_0203ffc4()) {
        func_ov194_021be648(wk, *(const u8 *)data, 1);
    }
}

static void func_ov194_021b8134(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != func_02040440()) {
        return;
    }
}

static void func_ov194_021b8140(int netId, int size, const void *data, void *work, NetHandle *handle) {
    PokemonTradeWork *wk = work;
    if (handle == func_02040440() && netId != func_0203ffc4()) {
        wk->unkFA0 = *(const u8 *)data;
    }
}

static void func_ov194_021b8164(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != func_02040440()) {
        return;
    }
}

static void func_ov194_021b8170(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The command only syncs the machines
    if (handle != func_02040440()) {
        return;
    }
}

static void PokemonTrade_WaitFadeOutToEnd(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        PokemonTrade_SetState(wk, NULL);
    }
}

void PokemonTrade_FadeOutToEnd(PokemonTradeWork *wk) {
    GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, PokemonTrade_WaitFadeOutToEnd);
}

void func_ov194_021b81c8(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    if (pkm != NULL) {
        func_ov194_021c24dc(wk, side);
        func_ov194_021c24ac(wk, side, 1 - side, pkm, 1, PokemonTrade_IsNegoType(wk));
        func_ov194_021c0918(wk, side, pkm);
    }
}

static void func_ov194_021b8200(PokemonTradeWork *wk, int side) {
    int i;
    if (!PokemonTrade_IsNegoType(wk)) {
        func_ov194_021c24dc(wk, side);
        func_ov194_021c123c(wk, side);
    } else {
        for (i = 0; i < 3; i++) {
            func_ov194_021c50d8(wk, side, i);
        }
    }
}

BOOL func_ov194_021b8234(PokemonTradeWork *wk) {
    u32 value;
    PartyPkm *pkm;
    if (!PokemonTrade_IsNegoType(wk)) {
        pkm = PokemonTrade_GetPkm(wk, 0);
        value = func_ov194_021b7c80(wk, 0);
    } else {
        pkm = PokemonTrade_GetPkm(wk, 1);
        value = func_ov194_021b7c80(wk, 1);
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

static BOOL func_ov194_021b82a8(PokemonTradeWork *wk, int box, int slot) {
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

BOOL func_ov194_021b8330(PokemonTradeWork *wk) {
    int side = 0;
    if (PokemonTrade_IsNegoType(wk)) {
        side = 1;
    }
    return PokemonTrade_IsBadPkm(PokemonTrade_GetPkm(wk, side));
}

BOOL func_ov194_021b8350(PokemonTradeWork *wk) {
    int side = 1;
    if (PokemonTrade_IsNegoType(wk)) {
        side = 0;
    }
    return PokemonTrade_IsBadPkm(PokemonTrade_GetPkm(wk, side));
}

BOOL func_ov194_021b8370(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    u32 value;
    if (!PokemonTrade_IsNegoType(wk)) {
        pkm = PokemonTrade_GetPkm(wk, 0);
        value = func_ov194_021b7c80(wk, 1);
    } else {
        pkm = PokemonTrade_GetPkm(wk, 1);
        value = func_ov194_021b7c80(wk, 0);
    }
    if (PML_ItemIsMail(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL)) && wk->unk11F4 != 0 && value == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov194_021b83d0(PokemonTradeWork *wk) {
    BOOL sent;
    BOOL network = PokemonTrade_IsNetwork(wk);
    if (!func_ov194_021c00b0(wk)) {
        return;
    }
    if (network) {
        if (func_ov194_021b8234(wk)) {
            sent = PokemonTrade_SendSelect(5);
        } else if (func_ov194_021b8330(wk)) {
            sent = PokemonTrade_SendSelect(7);
        } else if (func_ov194_021b8350(wk)) {
            sent = PokemonTrade_SendSelect(8);
        } else if (func_ov194_021b8370(wk)) {
            sent = PokemonTrade_SendSelect(6);
        } else {
            sent = PokemonTrade_SendSelect(2);
        }
        if (sent) {
            func_02040624(func_02040440(), 0x1b, 8);
            PokemonTrade_SetState(wk, func_ov194_021b94ec);
        }
    } else {
        wk->command[0] = 2;
        wk->command[1] = 2;
        PokemonTrade_SetState(wk, func_ov194_021b94ec);
    }
}

static void func_ov194_021b84b0(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
    func_ov194_021bfe28(wk);
    func_ov194_021bfe34(wk);
    PokemonTrade_SetState(wk, func_ov194_021b83d0);
}

static void func_ov194_021b84e0(PokemonTradeWork *wk, int side, BOOL a2) {
    if (side == 0) {
        ClActorPos pos = { 96, 16 };
        func_0204c140(wk->actors[4], &pos, 1);
    } else {
        ClActorPos pos = { 160, 16 };
        func_0204c140(wk->actors[4], &pos, 1);
    }
    if (func_0203d554()) {
        func_ov194_021c4970(wk, side, 0);
        func_0204c124(wk->actors[4], FALSE);
    } else {
        func_ov194_021c4970(wk, side, 1);
        func_0204c124(wk->actors[4], TRUE);
    }
    if (a2) {
        PartyPkm *pkm = PokemonTrade_GetPkm(wk, wk->cursor);
        func_ov194_021c0fa0(wk, pkm, wk->cursor, 1);
    }
}

static void func_ov194_021b8570(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        GFL_BGSysSetBGEnabled(3, FALSE);
        func_ov194_021c0aec(wk, 1);
        GFL_BGSysSetBGEnabled(4, FALSE);
        func_ov194_021c2c84(wk);
        func_ov194_021c200c(wk, 0);
        GFL_BGSysQueueScrLoad(3);
        G2_BlendNone();
        func_ov194_021b81c8(wk, 0, PokemonTrade_GetPkm(wk, 0));
        func_ov194_021b81c8(wk, 1, PokemonTrade_GetPkm(wk, 1));
        func_ov194_021b7898(wk);
        GFL_WipeSet(3, 1, 1, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, func_ov194_021b8838);
    }
}

static void func_ov194_021b860c(PokemonTradeWork *wk) {
    BOOL done = FALSE;
    if (wk->unk11F9 != 0 || !GFL_WipeIsFinished()) {
        return;
    }
    if (func_0203d554()) {
        if (PokemonTrade_GetPressedKeys()) {
            func_0203d564(FALSE);
            func_ov194_021b84e0(wk, wk->cursor, FALSE);
            return;
        }
    }
    GFL_BGSysSetBGEnabled(3, TRUE);
    switch (func_0203da0c(sSideButtonRects)) {
    case 0:
        if (wk->cursor != 0 || !func_0203d554()) {
            wk->cursor = 0;
            func_0203d564(TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov194_021b84e0(wk, wk->cursor, TRUE);
        }
        break;
    case 1:
        if (wk->cursor != 1 || !func_0203d554()) {
            wk->cursor = 1;
            func_0203d564(TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov194_021b84e0(wk, wk->cursor, TRUE);
        }
        break;
    case 2:
        done = TRUE;
        break;
    }
    if (PokemonTrade_GetPressedKeys() == PAD_KEY_LEFT || PokemonTrade_GetPressedKeys() == PAD_KEY_RIGHT) {
        wk->cursor = 1 - wk->cursor;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0203d564(FALSE);
        func_ov194_021b84e0(wk, wk->cursor, TRUE);
    }
    TouchBar_Main(wk->touchBar);
    if (TouchBar_GetDecided(wk->touchBar) == 1) {
        done = TRUE;
    }
    if (done) {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, func_ov194_021b8570);
    }
}

static void func_ov194_021b8754(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->cursor = 0;
        func_ov194_021c0b6c(wk, PokemonTrade_GetPkm(wk, 0));
        func_ov194_021b7898(wk);
        GFL_WipeSet(3, 1, 1, 0, 6, 1, wk->heapId);
        PokemonTrade_SetState(wk, func_ov194_021b860c);
    }
}

static void func_ov194_021b87b0(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        func_ov194_021bfe9c(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, func_ov194_021b84b0);
            break;
        case 1:
            GFL_WipeSet(3, 0, 0, 0, 6, 1, wk->heapId);
            PokemonTrade_SetState(wk, func_ov194_021b8754);
            break;
        case 2:
            PokemonTrade_SetState(wk, func_ov194_021b9b00);
            break;
        }
    }
}

static void func_ov194_021b8838(PokemonTradeWork *wk) {
    int i;
    GFL_MsgDataLoadStrbuf(wk->msgData, 17, wk->strbufTemplate);
    for (i = 0; i < 2; i++) {
        loadPokemonNicknameToStrbuf(wk->wordSet, i, PokemonTrade_GetPkm(wk, i));
    }
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
    func_ov194_021bfe28(wk);
    PokemonTrade_SetState(wk, func_ov194_021b8894);
}

static void func_ov194_021b8894(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        u32 items[] = { 4, 3, 5 };
        func_ov194_021c0214(wk, items, NELEMS(items));
        GFL_BGSysSetEnabledBGsA(0x1f);
        TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
        PokemonTrade_SetState(wk, func_ov194_021b87b0);
    }
}

static void func_ov194_021b88e4(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        if (func_02040664(func_02040440(), 0x14, 8)) {
            func_ov194_021bfe9c(wk);
            PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
        }
    } else {
        func_ov194_021bfe9c(wk);
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
    }
}

static void func_ov194_021b8924(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk) && wk->timer > 30) {
        if (PokemonTrade_IsNetwork(wk)) {
            func_02040624(func_02040440(), 0x14, 8);
        }
        PokemonTrade_SetState(wk, func_ov194_021b88e4);
    }
}

static void func_ov194_021b8960(PokemonTradeWork *wk) {
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
    func_ov194_021bc038(wk, 1, 1);
    if (wk->command[myId] == 1 && wk->command[otherId] == 1) {
        PokemonTrade_SetState(wk, func_ov194_021b8838);
        return;
    }
    if (wk->command[myId] == 4 && wk->command[otherId] == 4) {
        wk->param->next = 3;
        GFL_MsgDataLoadStrbuf(wk->msgData, 142, wk->strbuf);
        func_ov194_021bfe28(wk);
        func_ov194_021bfe34(wk);
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021b8924);
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
    PokemonTrade_SetState(wk, func_ov194_021b9c44);
}

static void func_ov194_021b8a5c(PokemonTradeWork *wk) {
    BOOL network = PokemonTrade_IsNetwork(wk);
    if (func_ov194_021c00b0(wk)) {
        if (!network) {
            wk->command[0] = 1;
            wk->command[1] = 1;
            PokemonTrade_SetState(wk, func_ov194_021b8960);
        } else if (PokemonTrade_SendSelect(1)) {
            func_02040624(func_02040440(), 0x1a, 8);
            PokemonTrade_SetState(wk, func_ov194_021b8960);
        }
    }
}

static void func_ov194_021b8ac0(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        wk->menu = NULL;
        if (choice == 0) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
            func_ov194_021bfe28(wk);
            func_ov194_021bfe34(wk);
            PokemonTrade_SetState(wk, func_ov194_021b8a5c);
        } else {
            PokemonTrade_SetState(wk, func_ov194_021ba7d4);
        }
    }
}

static void func_ov194_021b8b24(PokemonTradeWork *wk) {
    if (func_ov194_021b9df4(wk)) {
        u32 items[] = { 0, 5 };
        func_ov194_021c0214(wk, items, NELEMS(items));
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0x1b, -8);
        PokemonTrade_SetState(wk, func_ov194_021b8ac0);
    }
}

static void func_ov194_021b8b6c(PokemonTradeWork *wk) {
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    if (func_02042be8(func_02040440(), TRADE_NET_CMD_UNK16, 1, &inParty)) {
        PokemonTrade_SetState(wk, func_ov194_021b8b24);
    }
}

static void func_ov194_021b8bb4(PokemonTradeWork *wk) {
    TouchBar_SetIconVisible(wk->touchBar, 7, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 9, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
    func_0204c124(wk->actors[2], FALSE);
    func_ov194_021b79cc(wk);
    PokemonTrade_SetState(wk, func_ov194_021b8b6c);
}

static void func_ov194_021b8c08(PokemonTradeWork *wk, u32 a1, u32 a2) {
    wk->cursorColumn = a1;
    wk->cursorRow = a2;
    func_ov194_021b7898(wk);
}

static BOOL func_ov194_021b8c1c(PokemonTradeWork *wk, int *box, int *slot, int *a3, int *a4) {
    u32 x, y;
    int unused;
    if (func_0203dac8(&x, &y) == TRUE) {
        if (func_ov194_021c3fa8(wk, x + 12, y + 12, box, slot, a3, a4, &unused)) {
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
    if (func_0203dac8(&x, &y) == TRUE) {
        icon = func_ov194_021c3fa8(wk, x + 12, y + 12, &wk->heldBox, &wk->heldSlot, &a, &b, &c);
        if (icon != NULL) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->heldBox, wk->heldSlot, wk);
            if (func_ov194_021baac4(wk, wk->heldBox, wk->heldSlot)) {
                GFL_SndSEPlay(SEQ_SE_MSCL_05);
                if (!PokemonTrade_IsNegoType(wk)) {
                    PokemonTrade_SetState(wk, func_ov194_021ba708);
                    return 3;
                }
                wk->unkF98 = wk->heldBox;
                wk->unkF94 = wk->heldSlot;
                PokemonTrade_SetState(wk, func_ov194_021be4b0);
                return 3;
            }
            func_ov194_021b79e4(wk);
            if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
                if (func_ov194_021bbe60(wk, pkm)) {
                    GFL_MsgDataLoadStrbuf(wk->msgData, 143, wk->strbuf);
                    func_ov194_021bfdf8(wk, 1, 0);
                    PokemonTrade_SetState(wk, func_ov194_021b8dc4);
                    return 2;
                }
                func_ov194_021b7a2c(wk, icon, c, b, pkm);
                func_ov194_021b8c08(wk, a, b);
                if (a1) {
                    wk->unkF98 = wk->heldBox;
                    wk->unkF94 = wk->heldSlot;
                }
                func_ov194_021b7898(wk);
                ret = 1;
            }
        }
    }
    return ret;
}

static void func_ov194_021b8dc4(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk) && (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        wk->heldBox = -1;
        wk->heldSlot = -1;
        func_ov194_021bfe9c(wk);
        PokemonTrade_SetState(wk, func_ov194_021babc4);
        wk->firstColumn = -1;
        func_ov194_021c3c68(wk->boxes, wk, 1);
    }
}

static void func_ov194_021b8e2c(PokemonTradeWork *wk, u32 msg, BOOL a2) {
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->strbuf);
    func_ov194_021bfdf8(wk, a2, 0);
    wk->firstColumn = -1;
    func_ov194_021c3c68(wk->boxes, wk, 0);
    PokemonTrade_SetState(wk, func_ov194_021b8dc4);
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

static void func_ov194_021b8e98(PokemonTradeWork *wk) {
    PartyPkm *pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
    func_ov194_021c1288(wk, 0);
    func_ov194_021c0c04(wk, wk->cursor, pkm);
    if (func_0203d554()) {
        func_ov194_021b7898(wk);
    }
    GFL_HeapFree(pkm);
    PokemonTrade_SetState(wk, func_ov194_021b9058);
}

static void func_ov194_021b8ef4(PokemonTradeWork *wk) {
    if (func_ov194_021bc35c(wk, wk->selectSlot, wk->selectBox)) {
        PokemonTrade_SetState(wk, func_ov194_021ba924);
    }
}

static void func_ov194_021b8f1c(PokemonTradeWork *wk) {
    int index;
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    index = func_ov194_021bc178(wk, 0, wk->selectSlot, wk->selectBox);
    if (index != -1) {
        if (func_02042be8(func_02040440(), (u16)(TRADE_NET_CMD_UNK13 + index), 1, &inParty)) {
            PokemonTrade_SetState(wk, func_ov194_021b8ef4);
        }
    } else {
        PokemonTrade_SetState(wk, func_ov194_021b8ef4);
    }
}

static void func_ov194_021b8f94(PokemonTradeWork *wk) {
    if (func_ov194_021bc35c(wk, wk->selectSlot, wk->selectBox)) {
        PokemonTrade_SetState(wk, func_ov194_021be380);
    }
}

static void func_ov194_021b8fbc(PokemonTradeWork *wk) {
    int index;
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    index = func_ov194_021bc178(wk, 0, wk->selectSlot, wk->selectBox);
    if (index != -1) {
        if (func_02042be8(func_02040440(), (u16)(TRADE_NET_CMD_UNK13 + index), 1, &inParty)) {
            PokemonTrade_SetState(wk, func_ov194_021b8f94);
        }
    } else {
        PokemonTrade_SetState(wk, func_ov194_021b8f94);
    }
}

static void func_ov194_021b9034(PokemonTradeWork *wk) {
    u32 x, y;
    if (!func_0203da84(&x, &y)) {
        PokemonTrade_SetState(wk, func_ov194_021babc4);
    }
}

static void func_ov194_021b9058(PokemonTradeWork *wk) {
    u32 x, y;
    int a, b;
    BOOL selected = FALSE;
    BOOL showCursor = TRUE;
    int choice = -1;
    BOOL decided = FALSE;
    if (!AppTaskMenu_IsDecided(wk->menu)) {
        if (PokemonTrade_GetTouched()) {
            func_ov194_021b79e4(wk);
        }
        TouchBar_Main(wk->touchBar);
        if (TouchBar_GetTouched(wk->touchBar) == 9 && func_0203d554()) {
            AppTaskMenu_SetCursorActive(wk->menu, FALSE);
        }
        if (TouchBar_GetDecided(wk->touchBar) == 9) {
            selected = TRUE;
            wk->cursor = 1 - wk->cursor;
        }
        if (func_0203dac8(&x, &y) == TRUE) {
            func_0203d564(TRUE);
            AppTaskMenu_SetCursorActive(wk->menu, FALSE);
            if (func_ov194_021b8c1c(wk, &wk->unkF98, &wk->unkF94, &a, &b)) {
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
            func_ov194_021c1288(wk, 1);
            PokemonTrade_SetState(wk, func_ov194_021b8e98);
            return;
        }
        func_ov194_021b796c(wk);
        if (!decided && !func_0203da2c() && wk->touchHeld) {
            if (wk->heldIcon != NULL && wk->unkF5C) {
                GFL_SndSEPlay(SEQ_SE_SYS_56);
                choice = 0;
                decided = TRUE;
            } else {
                func_ov194_021b79e4(wk);
            }
        }
        func_ov194_021b78b4(wk);
        func_ov194_021b796c(wk);
    }
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        choice = AppTaskMenu_GetCursorPos(wk->menu);
        decided = TRUE;
    }
    if (decided) {
        if (choice == 0) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
            if (func_ov194_021baac4(wk, wk->unkF98, wk->unkF94) && PokemonTrade_IsNegoType(wk)) {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8f1c);
            } else if (hasPokemonChangedForm(pkm) == TRUE) {
                PokemonTrade_SetState(wk, func_ov194_021b8e80);
                func_ov194_021be554(wk, 1);
            } else if (func_ov194_021b82a8(wk, wk->unkF98, wk->unkF94)) {
                func_ov194_021be554(wk, 1);
                PokemonTrade_SetState(wk, func_ov194_021b8e74);
            } else if (func_ov194_021bbe60(wk, pkm)) {
                PokemonTrade_SetState(wk, func_ov194_021b8e8c);
                func_ov194_021be554(wk, 1);
            } else if (!PokemonTrade_IsNegoType(wk)) {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8bb4);
                showCursor = FALSE;
                TouchBar_SetIconVisible(wk->touchBar, 7, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, 9, FALSE);
            } else if (func_ov194_021bc04c(wk) && !func_0203d554()) {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8fbc);
                showCursor = FALSE;
                TouchBar_SetIconVisible(wk->touchBar, 7, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, 9, FALSE);
            } else {
                wk->selectSlot = wk->unkF94;
                wk->selectBox = wk->unkF98;
                PokemonTrade_SetState(wk, func_ov194_021b8f1c);
            }
        } else {
            func_ov194_021be554(wk, 1);
            PokemonTrade_SetState(wk, func_ov194_021b9034);
        }
        wk->unkF98 = -1;
        wk->unkF94 = -1;
        if (showCursor) {
            func_ov194_021ba8c0(wk);
            func_0204c124(wk->actors[2], TRUE);
        }
        func_ov194_021c4cfc(&wk->infoIcons[0]);
        func_ov194_021c4cfc(&wk->infoIcons[1]);
        func_ov194_021c4cfc(&wk->infoIcons[2]);
        func_ov194_021c1288(wk, 1);
        GFL_BGSysQueueScrLoad(6);
        func_ov194_021c2f78(wk);
        func_ov194_021c00fc(wk);
        wk->menu = NULL;
    }
    wk->touchHeld = func_0203da2c();
}

// The two sides' buttons
static const TouchRect sSideButtonRects[] = {
    { 4, 28, 88, 112 },
    { 4, 28, 152, 176 },
    { TOUCH_RECT_END },
};

static void func_ov194_021b9394(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    func_ov194_021be554(wk, 0);
    TouchBar_SetIconVisible(wk->touchBar, 1, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 7, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 9, TRUE);
    {
        u32 items[] = { 2, 5 };
        func_ov194_021c0214(wk, items, NELEMS(items));
    }
    pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
    wk->cursor = 0;
    func_ov194_021c0c04(wk, 0, pkm);
    GFL_HeapFree(pkm);
    func_0204c124(wk->actors[2], FALSE);
    func_ov194_021b79e4(wk);
    func_ov194_021b7898(wk);
    if (func_0203d554()) {
        AppTaskMenu_SetCursorActive(wk->menu, FALSE);
    } else {
        AppTaskMenu_SetCursorActive(wk->menu, TRUE);
    }
    PokemonTrade_SetState(wk, func_ov194_021b9058);
}

static void func_ov194_021b9454(PokemonTradeWork *wk) {
    u8 side = wk->cursor;
    if (!PokemonTrade_IsNetwork(wk) || func_02042be8(func_02040440(), TRADE_NET_CMD_UNK3, 1, &side)) {
        GXS_SetVisibleWnd(GX_WNDMASK_NONE);
        if (!PokemonTrade_IsNegoType(wk)) {
            PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Start);
        } else {
            PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Start);
        }
    }
}

void func_ov194_021b94c0(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || func_02040664(func_02040440(), 13, 8)) {
        PokemonTrade_SetState(wk, func_ov194_021b9454);
    }
}

static void func_ov194_021b94ec(PokemonTradeWork *wk) {
    int msg;
    int myId = PokemonTrade_GetMyNetId();
    int otherId = 1 - myId;
    int mine, other;
    if (PokemonTrade_IsNetwork(wk) && !func_02040664(func_02040440(), 0x1b, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 1, 1);
    if (wk->command[myId] == 2 && wk->command[otherId] == 2) {
        if (PokemonTrade_IsNetwork(wk)) {
            func_02040624(func_02040440(), 13, 8);
        }
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
    func_ov194_021bfe28(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    PokemonTrade_SetState(wk, func_ov194_021b9c44);
}

static void func_ov194_021b95ec(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNegoType(wk)) {
        func_ov194_021ba7d4(wk);
        func_ov194_021ba84c(wk);
        PokemonTrade_SetState(wk, func_ov194_021bb104);
    } else {
        func_ov194_021ba7d4(wk);
        func_ov194_021ba84c(wk);
    }
    GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
    GFL_BGSysSetEnabledBGsA(0x1f);
    GFL_BGSysSetEnabledBGsB(0x1e);
}

static void func_ov194_021b964c(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || func_02040664(func_02040440(), 18, 8)) {
        func_ov194_021bfe9c(wk);
        if (PokemonTrade_IsNegoType(wk)) {
            func_ov194_021bbf5c(wk);
            func_ov194_021be6ac(wk);
            if (wk->type == 2) {
                DWCRap_SetMic(FALSE);
            }
            GFL_MsgDataLoadStrbuf(wk->msgData, 135, wk->strbuf);
            func_ov194_021bfe28(wk);
            func_ov194_021bc2d0(wk, 0);
            func_ov194_021bc434(wk);
            wk->unk11E8[0] = 0;
            wk->unk11E8[1] = 0;
            func_ov194_021be534(wk);
            func_ov194_021be554(wk, 1);
            sys_memset(wk->iconSpecies, 0, sizeof(wk->iconSpecies));
            sys_memset(wk->iconForms, 0, sizeof(wk->iconForms));
            sys_memset(wk->iconSexes, 0, sizeof(wk->iconSexes));
            func_ov194_021b79e4(wk);
            if (wk->menu != NULL) {
                AppTaskMenu_Free(wk->menu);
                wk->menu = NULL;
            }
            func_ov194_021c3224(wk);
            func_ov194_021c5504(wk);
            func_ov194_021ba8c0(wk);
        }
        PokemonTrade_SetState(wk, func_ov194_021b95ec);
        PokemonTrade_Scroll(wk, 0, 1);
    }
}

static void func_ov194_021b9744(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk) && GFL_WipeIsFinished()) {
        func_ov194_021bfe9c(wk);
        PokemonTrade_SetState(wk, func_ov194_021b964c);
    }
}

static void func_ov194_021b976c(PokemonTradeWork *wk) {
    GFL_WipeSet(4, 0, 0, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, func_ov194_021b9744);
}

static void func_ov194_021b97a0(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk) || func_02040664(func_02040440(), 18, 8)) {
        func_ov194_021c2000(wk);
        PokemonTrade_SetState(wk, func_ov194_021b976c);
    }
}

static void func_ov194_021b97d0(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        if (func_02042c18(func_02040440(), 0xff, TRADE_NET_CMD_UNKE, sizeof(wk->boxColors[0]), &wk->boxColors[0], 0,
                          FALSE, TRUE)) {
            func_02040624(func_02040440(), 18, 8);
            PokemonTrade_SetState(wk, func_ov194_021b97a0);
        }
    } else {
        func_ov194_021c2000(wk);
        PokemonTrade_SetState(wk, func_ov194_021b97a0);
    }
}

static void func_ov194_021b9838(PokemonTradeWork *wk) {
    func_ov194_021bb3c0(wk, wk->timer - 1);
    if (wk->timer >= 24) {
        PokemonTrade_SetState(wk, func_ov194_021b97d0);
    }
}

static void func_ov194_021b9860(PokemonTradeWork *wk) {
    if (func_ov194_021c2c04(wk, wk->timer - 1)) {
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021b9838);
    }
}

static void func_ov194_021b9888(PokemonTradeWork *wk) {
    u8 data[2];
    if (PokemonTrade_IsNetwork(wk)) {
        data[0] = wk->boxCount;
        data[1] = func_ov194_021b7708(wk);
        if (func_02042be8(func_02040440(), TRADE_NET_CMD_UNK10, sizeof(data), data)) {
            wk->timer = 0;
            PokemonTrade_SetState(wk, func_ov194_021b9860);
        }
    } else {
        wk->partnerBoxCount = wk->boxCount;
        wk->unk107E = wk->partnerBoxCount * 160 + 16;
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021b9860);
    }
}

static void func_ov194_021b9904(PokemonTradeWork *wk) {
    func_ov194_021c3e9c(wk, wk->timer - 1);
    if (wk->timer > 25) {
        func_ov194_021c3c68(wk->boxes, wk, 0);
        PokemonTrade_SetState(wk, func_ov194_021b9888);
    }
}

static void func_ov194_021b993c(PokemonTradeWork *wk) {
    if (func_02040664(func_02040440(), 22, 8)) {
        func_02042e94(TRUE);
        func_02042e9c(TRUE);
        wk->timer = 0;
        PokemonTrade_SetState(wk, func_ov194_021b9904);
    }
}

static void func_ov194_021b9974(PokemonTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        if (PokemonTrade_IsNetwork(wk)) {
            func_02040624(func_02040440(), 22, 8);
            PokemonTrade_SetState(wk, func_ov194_021b993c);
        } else {
            PokemonTrade_SetState(wk, func_ov194_021b9888);
        }
    }
}

static void func_ov194_021b99b4(PokemonTradeWork *wk) {
    func_02042ba8(TRUE, wk->heapId);
    if (wk->type == 2) {
        DWCRap_SetMic(TRUE);
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
    func_ov194_021bfdf8(wk, TRUE, 0);
    func_ov194_021bfe34(wk);
    GFL_BGSysSetEnabledBGsA(0x10);
    GFL_BGSysSetEnabledBGsB(4);
    killBrightnessEitherEngine(0);
    GFL_WipeSet(4, 1, 1, 0, 6, 1, wk->heapId);
    PokemonTrade_SetState(wk, func_ov194_021b9974);
}

void func_ov194_021b9a38(PokemonTradeWork *wk) {
    int i;
    func_0203ffc4();
    wk->selectBox = -1;
    wk->selectSlot = -1;
    wk->unk800 = 0;
    wk->heldIcon = NULL;
    wk->unk1060 = 0;
    func_ov194_021c24dc(wk, 0);
    func_ov194_021c24dc(wk, 1);
    if (PokemonTrade_IsNegoType(wk)) {
        func_ov194_021bbf5c(wk);
    }
    sys_memset(wk->pkm[0], 0, PokeParty_GetPkmRawSize());
    sys_memset(wk->pkm[1], 0, PokeParty_GetPkmRawSize());
    for (i = 0; i < 10; i++) {
        if (wk->actors[i] != NULL) {
            func_0204c124(wk->actors[i], TRUE);
        }
    }
    func_ov194_021bfe9c(wk);
    PokemonTrade_SetState(wk, func_ov194_021b99b4);
}

static void func_ov194_021b9ad8(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        func_02040624(func_02040440(), 0x1b, 8);
        PokemonTrade_SetState(wk, func_ov194_021b94ec);
    }
}

static void func_ov194_021b9b00(PokemonTradeWork *wk) {
    u8 command = 3;
    if (!PokemonTrade_IsNetwork(wk)) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 97, wk->strbuf);
        func_ov194_021bfe28(wk);
        PokemonTrade_SetState(wk, func_ov194_021b9c44);
        return;
    }
    if (func_02042be8(func_02040440(), TRADE_NET_CMD_SELECT, 1, &command)) {
        func_02040624(func_02040440(), 0x1b, 8);
        GFL_MsgDataLoadStrbuf(wk->msgData, 6, wk->strbuf);
        func_ov194_021bfe28(wk);
        func_ov194_021bfe34(wk);
        PokemonTrade_SetState(wk, func_ov194_021b9ad8);
    }
}

static void func_ov194_021b9b80(PokemonTradeWork *wk) {
    int i;
    if (!func_02040664(func_02040440(), 0x19, 8)) {
        func_ov194_021bbff0(wk);
        return;
    }
    func_ov194_021bc038(wk, 1, 1);
    func_ov194_021bfe9c(wk);
    wk->command[0] = 0;
    wk->command[1] = 0;
    wk->unk11E8[0] = 0;
    wk->unk11E8[1] = 0;
    for (i = 0; i < 6; i++) {
        wk->negoSlot[i / 3][i % 3] = -1;
        if (wk->negoPkm[i / 3][i % 3] != NULL) {
            sys_memset(wk->negoPkm[i / 3][i % 3], 0, PokeParty_GetPkmRawSize());
        }
    }
    PokemonTrade_SetState(wk, func_ov194_021ba7d4);
}

static void func_ov194_021b9c1c(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        func_02040624(func_02040440(), 0x19, 8);
        PokemonTrade_SetState(wk, func_ov194_021b9b80);
    }
}

void func_ov194_021b9c44(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk) && (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched())) {
        func_ov194_021bfe9c(wk);
        if (PokemonTrade_IsNetwork(wk)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 50, wk->strbuf);
            func_ov194_021bfe28(wk);
            func_ov194_021bfe34(wk);
            PokemonTrade_SetState(wk, func_ov194_021b9c1c);
        } else {
            PokemonTrade_SetState(wk, func_ov194_021ba7d4);
        }
    }
}

static void func_ov194_021b9cbc(PokemonTradeWork *wk) {
    NetHandle *handle = func_02040440();
    if (func_ov194_021c00b0(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            u8 command = 2;
            if (func_02042be8(handle, TRADE_NET_CMD_UNKF, 1, &command)) {
                PokemonTrade_SetState(wk, func_ov194_021b8960);
            }
        } else {
            PokemonTrade_SetState(wk, func_ov194_021b8960);
        }
    }
}

static void func_ov194_021b9d0c(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNetwork(wk)) {
        if (PokemonTrade_SendSelect(4)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 137, wk->strbuf);
            func_ov194_021bfe28(wk);
            func_ov194_021bfe34(wk);
            func_02040624(func_02040440(), 0x1a, 8);
            PokemonTrade_SetState(wk, func_ov194_021b9cbc);
        }
    } else {
        wk->param->next = 3;
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
    }
}

static void func_ov194_021b9d84(PokemonTradeWork *wk) {
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        u8 choice = AppTaskMenu_GetCursorPos(wk->menu);
        func_ov194_021c00fc(wk);
        wk->menu = NULL;
        switch (choice) {
        case 0:
            PokemonTrade_SetState(wk, func_ov194_021b9d0c);
            break;
        case 1:
            func_ov194_021bfe9c(wk);
            PokemonTrade_SetState(wk, func_ov194_021babc4);
            wk->firstColumn = -1;
            func_ov194_021c3c68(wk->boxes, wk, 0);
            func_ov194_021c5504(wk);
            break;
        }
    }
}

static BOOL func_ov194_021b9df4(PokemonTradeWork *wk) {
    PartyPkm *pkm = PokemonTrade_CopyPartyPkm(wk->boxes, wk->selectBox, wk->selectSlot, wk);
    if (PokemonTrade_IsNetwork(wk) &&
        !func_02042be8(func_02040440(), TRADE_NET_CMD_UNK1, PokeParty_GetPkmRawSize(), pkm)) {
        GFL_HeapFree(pkm);
        return FALSE;
    }
    func_ov194_021b81c8(wk, 0, pkm);
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

static void func_ov194_021b9e60(PokemonTradeWork *wk) {
    if (func_ov194_021c00b0(wk)) {
        u32 items[] = { 24, 25 };
        func_ov194_021c0120(wk, items, NELEMS(items), 32, 12);
        wk->firstColumn = -1;
        func_ov194_021c3c68(wk->boxes, wk, 0);
        PokemonTrade_SetState(wk, func_ov194_021b9d84);
    }
}

static void func_ov194_021b9eb8(PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, 141, wk->strbuf);
    func_ov194_021bfe28(wk);
    wk->unk1074 = 1;
    PokemonTrade_SetState(wk, func_ov194_021b9e60);
}

static void func_ov194_021b9eec(PokemonTradeWork *wk) {
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0x1b, -8);
    PokemonTrade_SetState(wk, func_ov194_021b9eb8);
}

static BOOL func_ov194_021b9f10(PokemonTradeWork *wk, u32 a1) {
    int slot;
    BoxPkm *pkm;
    int column = func_ov194_021c3bc0(wk);
    int i;
    for (i = 0; i < wk->columnCount; i++) {
        column++;
        if (column >= wk->columnCount) {
            column = 0;
        }
        for (slot = 0; slot < 5; slot++) {
            int box = PokemonTrade_GetColumnBox(column, wk);
            if ((pkm = PokemonTrade_GetBoxPkm(wk->boxes, box, PokemonTrade_GetColumnSlot(column, slot), wk)) != NULL &&
                func_ov194_021c38a8(PML_PkmGetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL), a1) &&
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

static void func_ov194_021b9fcc(PokemonTradeWork *wk) {
    GFL_BGSysSetEnabledBGsB(0x1e);
    PokemonTrade_SetState(wk, func_ov194_021babc4);
}

static void func_ov194_021b9fe4(PokemonTradeWork *wk) {
    GFL_BGSysSetEnabledBGsB(0x1a);
    func_ov194_021c4b88(wk);
    func_ov194_021bfe9c(wk);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    func_ov194_021be554(wk, 1);
    func_ov194_021c5fe4(wk, 0);
    func_ov194_021ba8c0(wk);
    func_ov194_021c5504(wk);
    func_ov194_021c5e5c(wk);
    PokemonTrade_SetState(wk, func_ov194_021b9fcc);
}

static void func_ov194_021ba03c(PokemonTradeWork *wk) {
    if (!func_ov194_021c5e80(wk)) {
        PokemonTrade_SetState(wk, func_ov194_021b9fe4);
    }
}

static void func_ov194_021ba058(PokemonTradeWork *wk) {
    GFL_BGSysSetEnabledBGsB(0x1f);
    if (func_ov194_021c00b0(wk)) {
        int box = func_0203da0c(sBoxListRects);
        if (box != TOUCH_RECT_NONE) {
            wk->unk800 = box + 1;
            func_ov194_021c5dd8(wk, sBoxListRects[box].left, sBoxListRects[box].top);
            if (func_ov194_021b9f10(wk, box)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                GXS_SetVisibleWnd(GX_WNDMASK_NONE);
                PokemonTrade_SetState(wk, func_ov194_021ba03c);
                return;
            }
            GFL_MsgDataLoadStrbuf(wk->msgData, 52, wk->strbuf);
            func_ov194_021bfcf8(wk, 1, 1, 1, 29, 4);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        TouchBar_Main(wk->touchBar);
        if (TouchBar_GetDecided(wk->touchBar) == 1) {
            GFL_BGSysSetEnabledBGsB(0x1a);
            wk->unk800 = 0;
            func_ov194_021c4b88(wk);
            func_ov194_021bfe9c(wk);
            GXS_SetVisibleWnd(GX_WNDMASK_NONE);
            func_ov194_021be554(wk, 1);
            func_ov194_021c5fe4(wk, 0);
            func_ov194_021ba8c0(wk);
            func_ov194_021c5504(wk);
            func_ov194_021c5e5c(wk);
            PokemonTrade_SetState(wk, func_ov194_021b9fe4);
        }
    }
}

static void func_ov194_021ba170(PokemonTradeWork *wk) {
    func_ov194_021c4a68(wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 51, wk->strbuf);
    func_ov194_021bfcf8(wk, 1, 1, 1, 29, 4);
    PokemonTrade_SetState(wk, func_ov194_021ba058);
}

static void func_ov194_021ba1b0(PokemonTradeWork *wk) {
    TouchBar_SetIconActive(wk->touchBar, 7, FALSE);
    func_ov194_021be554(wk, 0);
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 10, -8);
    func_ov194_021c5fe4(wk, 1);
    if (func_ov194_021bc098(wk)) {
        TouchBar_SetIconActive(wk->touchBar, 8, FALSE);
    }
    PokemonTrade_SetState(wk, func_ov194_021ba170);
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

static BOOL func_ov194_021ba240(PokemonTradeWork *wk, int column) {
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

static void func_ov194_021ba280(PokemonTradeWork *wk) {
    int i;
    int column;
    BOOL moved = FALSE;
    u32 keys = GCTX_HIDGetTypedKeys();
    if (wk->heldIcon != NULL) {
        return;
    }
    if (keys & PAD_BUTTON_R) {
        func_0203d564(FALSE);
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
        func_ov194_021bbdac(wk, moved);
    } else if (keys & PAD_BUTTON_L) {
        func_0203d564(FALSE);
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
        func_ov194_021bbdac(wk, moved);
    } else if (GCTX_HIDGetTypedKeys() == PAD_KEY_UP) {
        func_0203d564(FALSE);
        if (!func_ov194_021c3c10(wk, &column)) {
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
        func_0203d564(FALSE);
        if (!func_ov194_021c3c10(wk, &column)) {
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
        func_0203d564(FALSE);
        moved = TRUE;
        wk->firstColumn--;
        if (!func_ov194_021c3c10(wk, &column)) {
            wk->cursorColumn = column;
        } else {
            wk->cursorColumn++;
            if (!func_ov194_021ba240(wk, wk->cursorColumn)) {
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
        func_0203d564(FALSE);
        wk->firstColumn++;
        moved = TRUE;
        if (!func_ov194_021c3c10(wk, &column)) {
            wk->cursorColumn = column;
        } else {
            wk->cursorColumn--;
            if (wk->cursorColumn < 0) {
                wk->cursorColumn = wk->columnCount - 1;
            }
            if (!func_ov194_021ba240(wk, wk->cursorColumn)) {
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
    func_ov194_021c3c68(wk->boxes, wk, 1);
    func_ov194_021c30b8(wk);
    func_ov194_021c339c(wk);
    if (!GFL_SndPlayerIsActiveAny() && playSound) {
        GFL_SndSEPlay(SEQ_SE_SYS_57);
    }
    if (PokemonTrade_IsNetwork(wk) && send) {
        if (func_02042b20()) {
            if (wk->scrollX % 16 == 0) {
                func_02042c18(func_02040440(), 0xff, TRADE_NET_CMD_UNKC, sizeof(wk->scrollX), &wk->scrollX, 0, TRUE,
                              TRUE);
            }
        } else {
            func_02042c18(func_02040440(), 0xff, TRADE_NET_CMD_UNKC, sizeof(wk->scrollX), &wk->scrollX, 0, TRUE, TRUE);
        }
    } else if (!PokemonTrade_IsNetwork(wk)) {
        wk->unk107E = wk->scrollX;
    }
}

static void func_ov194_021ba5f0(PokemonTradeWork *wk) {
    u32 x, y;
    u32 i;
    if (wk->heldIcon != NULL) {
        return;
    }
    if (func_0203da84(&x, &y)) {
        if (wk->unk1088) {
            if (x >= 64 && x < 192 && y >= 168 && y < 188) {
                func_0204c520(wk->actors[2], TRUE);
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
    if (!func_0203da2c() && wk->scrollSpeed != 0) {
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

static void func_ov194_021ba6ec(PokemonTradeWork *wk) {
    if (!func_0203da2c()) {
        PokemonTrade_SetState(wk, func_ov194_021babc4);
    }
}

static void func_ov194_021ba708(PokemonTradeWork *wk) {
    func_ov194_021bc0f0(wk, wk->unkF98, wk->unkF94);
    if (!PokemonTrade_IsNetwork(wk) || func_02042be8(func_02040440(), TRADE_NET_CMD_UNK6, 0, NULL)) {
        TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
        wk->selectSlot = -1;
        wk->selectBox = -1;
        wk->heldSlot = -1;
        wk->heldBox = -1;
        wk->unkF94 = -1;
        wk->unkF98 = -1;
        func_ov194_021b8200(wk, 0);
        func_ov194_021b7898(wk);
        PokemonTrade_SetState(wk, func_ov194_021ba6ec);
    }
}

static void func_ov194_021ba78c(PokemonTradeWork *wk) {
    if (!PokemonTrade_IsNetwork(wk)) {
        func_ov194_021b8200(wk, 1);
        func_ov194_021b7be0(wk);
        PokemonTrade_SetState(wk, func_ov194_021ba84c);
    } else if (func_02042be8(func_02040440(), TRADE_NET_CMD_UNK6, 0, NULL)) {
        PokemonTrade_SetState(wk, func_ov194_021ba84c);
    }
}

static void func_ov194_021ba7d4(PokemonTradeWork *wk) {
    wk->unk1088 = 0;
    wk->scrollTouchX = 0;
    wk->scrollSpeed = 0;
    sys_memset(wk->iconSpecies, 0, sizeof(wk->iconSpecies));
    sys_memset(wk->iconForms, 0, sizeof(wk->iconForms));
    sys_memset(wk->iconSexes, 0, sizeof(wk->iconSexes));
    func_ov194_021b79e4(wk);
    func_ov194_021be554(wk, 1);
    if (wk->menu != NULL) {
        AppTaskMenu_Free(wk->menu);
        wk->menu = NULL;
    }
    func_ov194_021b8200(wk, 0);
    PokemonTrade_SetState(wk, func_ov194_021ba78c);
}

static void func_ov194_021ba84c(PokemonTradeWork *wk) {
    TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 7, TRUE);
    TouchBar_SetIconActive(wk->touchBar, 7, TRUE);
    TouchBar_SetIconVisible(wk->touchBar, 9, FALSE);
    func_ov194_021b76e0(wk);
    func_0204c124(wk->actors[2], TRUE);
    func_ov194_021c3224(wk);
    func_ov194_021c5504(wk);
    wk->selectSlot = -1;
    wk->selectBox = -1;
    func_ov194_021b7898(wk);
    PokemonTrade_SetState(wk, func_ov194_021babc4);
}

static void func_ov194_021ba8c0(PokemonTradeWork *wk) {
    func_0204c124(wk->actors[2], TRUE);
    TouchBar_SetIconVisible(wk->touchBar, 8, func_ov194_021bc098(wk));
    TouchBar_SetIconActive(wk->touchBar, 8, func_ov194_021bc098(wk));
    TouchBar_SetIconVisible(wk->touchBar, 7, TRUE);
    TouchBar_SetIconActive(wk->touchBar, 7, TRUE);
    TouchBar_SetIconVisible(wk->touchBar, 9, FALSE);
    func_ov194_021b76e0(wk);
    func_ov194_021b7898(wk);
}

void func_ov194_021ba924(PokemonTradeWork *wk) {
    func_ov194_021be554(wk, 1);
    sys_memset(wk->iconSpecies, 0, sizeof(wk->iconSpecies));
    sys_memset(wk->iconForms, 0, sizeof(wk->iconForms));
    sys_memset(wk->iconSexes, 0, sizeof(wk->iconSexes));
    func_ov194_021b79e4(wk);
    if (wk->menu != NULL) {
        AppTaskMenu_Free(wk->menu);
        wk->menu = NULL;
    }
    func_ov194_021c3224(wk);
    func_ov194_021c5504(wk);
    func_ov194_021ba8c0(wk);
    PokemonTrade_SetState(wk, func_ov194_021babc4);
}

static void func_ov194_021ba990(PokemonTradeWork *wk) {
    if (PokemonTrade_IsNegoType(wk)) {
        if (!func_ov194_021bc35c(wk, wk->selectSlot, wk->selectBox)) {
            return;
        }
    } else if (!func_ov194_021b9df4(wk)) {
        return;
    }
    wk->unkF98 = -1;
    wk->unkF94 = -1;
    wk->heldSlot = -1;
    wk->heldBox = -1;
    func_ov194_021b7898(wk);
    PokemonTrade_SetState(wk, func_ov194_021babc4);
}

static void func_ov194_021ba9ec(PokemonTradeWork *wk) {
    int index;
    u32 inParty = FALSE;
    if (wk->selectBox == wk->boxCount) {
        inParty = TRUE;
    }
    if (PokemonTrade_IsNegoType(wk)) {
        index = func_ov194_021bc178(wk, 0, wk->selectSlot, wk->selectBox);
        if (index != -1) {
            if (func_02042be8(func_02040440(), (u16)(TRADE_NET_CMD_UNK13 + index), 1, &inParty)) {
                PokemonTrade_SetState(wk, func_ov194_021ba990);
            }
        } else {
            PokemonTrade_SetState(wk, func_ov194_021ba990);
        }
    } else if (func_02042be8(func_02040440(), TRADE_NET_CMD_UNK16, 1, &inParty)) {
        PokemonTrade_SetState(wk, func_ov194_021ba990);
    }
}

static void func_ov194_021baa90(PokemonTradeWork *wk) {
    if (func_ov194_021c5460(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            PokemonTrade_SetState(wk, func_ov194_021ba9ec);
        } else {
            PokemonTrade_SetState(wk, func_ov194_021ba990);
        }
    }
}

static BOOL func_ov194_021baac4(PokemonTradeWork *wk, int box, int slot) {
    if (!PokemonTrade_IsNegoType(wk)) {
        if (wk->selectSlot == slot && wk->selectBox == box) {
            return TRUE;
        }
    } else if (func_ov194_021bc0f0(wk, box, slot) != -1) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov194_021bab08(PokemonTradeWork *wk) {
    BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
    if (func_ov194_021baac4(wk, wk->unkF98, wk->unkF94)) {
        GFL_SndSEPlay(SEQ_SE_MSCL_05);
        if (!PokemonTrade_IsNegoType(wk)) {
            PokemonTrade_SetState(wk, func_ov194_021ba708);
            return TRUE;
        }
        PokemonTrade_SetState(wk, func_ov194_021be4b0);
        return TRUE;
    }
    if (pkm != NULL && PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
        GFL_SndSEPlay(SEQ_SE_SELECT4);
        TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
        func_ov194_021c00fc(wk);
        PokemonTrade_SetState(wk, func_ov194_021b9394);
        return TRUE;
    }
    func_ov194_021b79e4(wk);
    wk->unkF98 = -1;
    wk->unkF94 = -1;
    return FALSE;
}

static void func_ov194_021babc4(PokemonTradeWork *wk) {
    int column;
    wk->unk11EA = 1;
    if (func_0203da2c()) {
        func_0203d564(TRUE);
        if (GCTX_HIDGetHeldKeys()) {
            return;
        }
    }
    if (func_0203d554() && PokemonTrade_GetPressedKeys()) {
        func_0203d564(FALSE);
        if (!func_ov194_021c3c10(wk, &column)) {
            wk->cursorColumn = column;
            if (wk->cursorColumn < 2 && wk->cursorRow >= 3) {
                wk->cursorRow = 2;
            }
        } else if (!func_ov194_021ba240(wk, wk->cursorColumn)) {
            int first = func_ov194_021c3bc0(wk);
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
        func_ov194_021b7898(wk);
        wk->scrollSpeed = 0;
        return;
    }
    if (PokemonTrade_GetPressedKeys() || PokemonTrade_GetTouched()) {
        wk->scrollSpeed = 0;
    }
    if (PokemonTrade_GetTouched()) {
        func_ov194_021b79e4(wk);
        wk->unkF98 = -1;
        wk->unkF94 = -1;
    }
    func_ov194_021b78b4(wk);
    func_ov194_021b796c(wk);
    func_ov194_021ba280(wk);
    func_ov194_021be598(wk);
    if (PokemonTrade_GetPressedKeys() == PAD_BUTTON_A) {
        BoxPkm *pkm;
        int box;
        func_0203d564(FALSE);
        if (!func_ov194_021c3c10(wk, &column)) {
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
            if (func_ov194_021bbe60(wk, pkm)) {
                GFL_MsgDataLoadStrbuf(wk->msgData, 143, wk->strbuf);
                func_ov194_021bfdf8(wk, 1, 0);
                PokemonTrade_SetState(wk, func_ov194_021b8dc4);
                return;
            }
            wk->unkF98 = PokemonTrade_GetColumnBox(wk->cursorColumn, wk);
            wk->unkF94 = PokemonTrade_GetColumnSlot(wk->cursorColumn, wk->cursorRow);
            func_0203d564(FALSE);
            if (PokemonTrade_IsNegoType(wk) && func_ov194_021bc0f0(wk, wk->unkF98, wk->unkF94) != -1) {
                GFL_SndSEPlay(SEQ_SE_MSCL_05);
                PokemonTrade_SetState(wk, func_ov194_021be4b0);
                return;
            }
            pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->unkF98, wk->unkF94, wk);
            func_ov194_021b79e4(wk);
            func_ov194_021c3820(wk);
            func_ov194_021bab08(wk);
            return;
        }
    }
    switch (func_ov194_021b8c84(wk, FALSE)) {
    case 2:
        return;
    case 3:
        return;
    case 1:
        func_0203d564(TRUE);
        wk->unkF70 = 1;
        break;
    }
    if (!func_0203da2c()) {
        if (wk->unk1088 && wk->unkF5C && wk->heldIcon != NULL) {
            BoxPkm *pkm = PokemonTrade_GetBoxPkm(wk->boxes, wk->heldBox, wk->heldSlot, wk);
            wk->unk1088 = 0;
            wk->unkF5C = 0;
            if (hasPokemonChangedForm(pkm) == TRUE) {
                wk->heldBox = -1;
                wk->heldSlot = -1;
                func_ov194_021b79cc(wk);
                PokemonTrade_SetState(wk, func_ov194_021b8e80);
                return;
            }
            if (func_ov194_021b82a8(wk, wk->heldBox, wk->heldSlot)) {
                wk->heldBox = -1;
                wk->heldSlot = -1;
                func_ov194_021b79cc(wk);
                PokemonTrade_SetState(wk, func_ov194_021b8e74);
                return;
            }
            wk->unkF98 = wk->heldBox;
            wk->unkF94 = wk->heldSlot;
            wk->selectSlot = wk->heldSlot;
            wk->selectBox = wk->heldBox;
            wk->heldIcon = NULL;
            TouchBar_SetIconVisible(wk->touchBar, 8, TRUE);
            TouchBar_SetIconActive(wk->touchBar, 8, TRUE);
            func_ov194_021c5348(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_56);
            PokemonTrade_SetState(wk, func_ov194_021baa90);
            return;
        }
        if (wk->unk1088 && wk->heldIcon != NULL && wk->unkF70) {
            wk->unkF98 = wk->heldBox;
            wk->unkF94 = wk->heldSlot;
            func_ov194_021bab08(wk);
            return;
        }
        if (wk->heldSlot != -1 && wk->heldBox != -1) {
            func_ov194_021b79e4(wk);
            func_ov194_021b7898(wk);
        } else {
            func_ov194_021b79e4(wk);
        }
        wk->unk1088 = 0;
        wk->unkF5C = 0;
        func_0204c520(wk->actors[2], FALSE);
    }
    func_ov194_021ba5f0(wk);
    if (func_0203da2c()) {
        wk->unk1088 = 1;
    }
    TouchBar_Main(wk->touchBar);
    {
        u32 button = TouchBar_GetTouched(wk->touchBar);
        switch (button) {
        case 1:
        case 7:
        case 8:
            PokemonTrade_SetState(wk, func_ov194_021bb044);
            break;
        }
    }
}

static void func_ov194_021bb044(PokemonTradeWork *wk) {
    TouchBar_Main(wk->touchBar);
    switch (TouchBar_GetDecided(wk->touchBar)) {
    case 1:
        PokemonTrade_SetState(wk, func_ov194_021b9eec);
        break;
    case 7:
        PokemonTrade_SetState(wk, func_ov194_021ba1b0);
        break;
    case 8:
        if (PokemonTrade_IsNegoType(wk)) {
            if (func_ov194_021bc098(wk)) {
                TouchBar_SetIconVisible(wk->touchBar, 7, FALSE);
                TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
                func_ov194_021b79cc(wk);
                PokemonTrade_SetState(wk, func_ov194_021be380);
                return;
            }
            TouchBar_SetIconVisible(wk->touchBar, 7, TRUE);
            TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
        } else if (wk->selectSlot == -1) {
            TouchBar_SetIconVisible(wk->touchBar, 7, TRUE);
            TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
        } else {
            PokemonTrade_SetState(wk, func_ov194_021b8bb4);
            return;
        }
        PokemonTrade_SetState(wk, func_ov194_021babc4);
        break;
    }
}

static void func_ov194_021bb104(PokemonTradeWork *wk) {
    BOOL done = FALSE;
    if (func_ov194_021c00b0(wk)) {
        if (PokemonTrade_GetPressedKeys()) {
            func_0203d564(FALSE);
            done = TRUE;
        }
        if (PokemonTrade_GetTouched()) {
            func_0203d564(TRUE);
            done = TRUE;
        }
        if (done) {
            func_ov194_021bfe9c(wk);
            func_ov194_021c5504(wk);
            PokemonTrade_SetState(wk, func_ov194_021babc4);
            wk->firstColumn = -1;
            func_ov194_021c3c68(wk->boxes, wk, 1);
        }
    }
}

static void func_ov194_021bb180(PokemonTradeWork *wk) {
    func_ov194_021c43c0(wk);
    wk->mcssSys = MCSSSys_Create(4, HEAPID_TAIL(wk->heapId));
    func_0201aefc(wk->mcssSys, 0x30000);
    func_0201aacc(wk->mcssSys);
}

static void func_ov194_021bb1bc(PokemonTradeWork *wk) {
    func_ov194_021c24dc(wk, 0);
    func_ov194_021c24dc(wk, 1);
    MCSSSys_Free(wk->mcssSys);
    wk->mcssSys = NULL;
    GFL_G3DCameraFree(wk->camera);
    GFL_G3DSysFree();
}

static void func_ov194_021bb1ec(PokemonTradeWork *wk) {
    GFL_BGSysCreate(HEAPID_POKEMON_TRADE);
    BmpWin_InitAllocator(wk->heapId);
    func_ov194_021c41fc(wk);
    func_ov194_021c4234(wk);
    func_ov194_021c4484(wk);
}

static void func_ov194_021bb218(PokemonTradeWork *wk) {
    func_0204b758();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void func_ov194_021bb228(PokemonTradeWork *wk) {
    func_ov194_021c2e6c(wk);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    func_ov194_021be6ac(wk);
    if (wk->type < 4) {
        func_ov194_021c46a4(wk);
    }
    func_ov194_021c3480(wk);
    func_ov194_021c2d78(wk);
    func_ov194_021c2c84(wk);
    func_ov194_021c3224(wk);
    func_ov194_021c3c68(wk->boxes, wk, 0);
}

static void PokemonTrade_VBlank(TCB *tcb, void *data) {
    PokemonTradeWork *wk = data;
    func_0204b7c8();
    if (wk->unk1084) {
        GFL_BGSysMoveBG(7, 0, wk->unk108C);
        GFL_BGSysMoveBG(5, 0, wk->unk108C);
        wk->unk1084 = 0;
    }
}

static void func_ov194_021bb2cc(PokemonTradeWork *wk, GameData *gameData, u16 friendIndex) {
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
                wk->partnerInfo = func_02017378(gameData, 1 - func_0203ffc4());
            } else {
                wk->partnerInfo = func_02017378(gameData, 1);
            }
        }
    }
}

static void func_ov194_021bb364(PokemonTradeWork *wk, BOOL a1) {
    u32 i;
    for (i = 0; i < 3; i++) {
        GFL_HeapFree(wk->recvPkm[i]);
    }
}

static void func_ov194_021bb384(PokemonTradeWork *wk) {
    int count = BoxSaveAccessor_GetAvailableBoxCount(wk->boxes);
    wk->columnCount = count * 6 + 2;
    wk->boxCount = count;
    wk->stripTileWidth = count * 20 + 12;
    wk->stripWidth = count * 160 + 96;
}

// Fills the icons of the party (with the first box) or a box
static void func_ov194_021bb3c0(PokemonTradeWork *wk, int box) {
    int i;
    BoxPkm *pkm;
    TradeBoxEntry *entries = wk->boxEntries;
    if (box == 0) {
        sys_memset(entries, 0, sizeof(wk->boxEntries));
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

void func_ov194_021bb4b4(PokemonTradeWork *wk) {
    int i;
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    func_ov194_021c52bc(wk);
    func_ov194_021c2034(wk);
    func_ov194_021bc330(wk);
    func_ov194_021be688(wk);
    func_ov194_021bc784(wk);
    func_ov194_021c3374(wk);
    func_ov194_021c24dc(wk, 1);
    func_ov194_021c57a8(wk);
    func_ov194_021be578(wk);
    func_ov194_021c1484(wk);
    func_ov194_021c368c(wk);
    func_ov194_021c123c(wk, 0);
    func_ov194_021c123c(wk, 1);
    if (wk->touchBar != NULL) {
        TouchBar_Free(wk->touchBar);
        wk->touchBar = NULL;
    }
    func_ov194_021c36e4(wk);
    if (wk->clactUnit != NULL) {
        func_0204bf98(wk->clactUnit);
        wk->clactUnit = NULL;
    }
    func_ov194_021bffac(wk);
    func_ov194_021bbf18(wk);
    func_ov194_021c1740(wk, 0);
    func_ov194_021c1740(wk, 1);
    func_ov194_021c2d0c(wk);
    func_ov194_021c2e04(wk);
    func_ov194_021c45a8(wk);
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
    sys_memset(wk, 0, sizeof(PokemonTradeWork));
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
        func_ov194_021bb2cc(wk, param->gameData, param->unk2E);
    }
    func_ov194_021bb384(wk);
    func_ov194_021bb180(wk);
    func_ov194_021bb1ec(wk);
    wk->clactUnit = func_0204bf1c(340, 0, wk->heapId);
    if (PokemonTrade_IsNetwork(wk)) {
        func_02040c20(TRADE_NET_CMD_BASE, sNetCommands, NELEMS(sNetCommands), wk);
        func_02042e94(TRUE);
        func_02042e9c(TRUE);
    }
    wk->vblankTcb = GFL_VBlankTCBAdd(PokemonTrade_VBlank, wk, 0);
    wk->heldIcon = NULL;
    wk->selectSlot = -1;
    wk->sceneId = -1;
    wk->heldSlot = -1;
    wk->heldBox = -1;
    wk->unkF98 = -1;
    wk->unkF94 = -1;
    func_ov194_021c1ef0(wk);
    if (param->next == 1) {
        GFL_BGSysSetEnabledBGsA(0x10);
        PokemonTrade_SetState(wk, func_ov194_021bf938);
        param->next = 0;
        return TRUE;
    }
    func_ov194_021bfedc(wk);
    func_ov194_021c2de8(wk);
    if (wk->type < 4) {
        func_ov194_021bb228(wk);
        func_ov194_021c2a24(wk);
        func_02042ba8(TRUE, wk->heapId);
    }
    func_ov194_021c200c(wk, 0);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    PokemonTrade_SetState(wk, func_ov194_021b99b4);
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

static BOOL func_ov194_021bb868(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeParam *trade = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    int i;
    for (i = 0; i < 2; i++) {
        wk->unk1048[i] = 0;
        wk->unk1050[i] = 1;
    }
    wk->partnerInfo = func_02017378(trade->gameData, 1 - func_0203ffc4());
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

static void func_ov194_021bb938(PokemonTradeDemoParam *demo, PokemonTradeWork *wk) {
    sys_memcpy(demo->pkm[0], wk->pkm[0], PokeParty_GetPkmRawSize());
    sys_memcpy(demo->pkm[1], wk->pkm[1], PokeParty_GetPkmRawSize());
    func_ov194_021c24cc(wk, 0, 1, demo->pkm[0], 1);
    func_ov194_021c24cc(wk, 1, 1, demo->pkm[1], 1);
    MCSS_Hide(!PokemonTrade_IsNegoType(wk) ? wk->mcss[1] : wk->mcss[0]);
    wk->myInfo = demo->myInfo;
    wk->partnerInfo = demo->partnerInfo;
    func_02042ba8(FALSE, wk->heapId);
    GFL_FadeSet(3, 16, 0, 2);
}

static BOOL func_ov194_021bb9cc(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0xc6000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 4);
    func_ov194_021bb938(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeIrcDemo_Start);
    return ret;
}

static BOOL func_ov194_021bba0c(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0x96000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 7);
    func_ov194_021bb938(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeDemoDown_Start);
    return ret;
}

static BOOL func_ov194_021bba4c(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0x96000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 8);
    func_ov194_021bb938(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeDemoMiddle_Start);
    return ret;
}

static BOOL func_ov194_021bba8c(GameProc *proc, u32 *state, void *param, void *work) {
    PokemonTradeDemoParam *demo = param;
    PokemonTradeWork *wk = PokemonTrade_CreateWork(proc, 0x96000);
    BOOL ret;
    demo->trade.gameData = demo->gameData;
    ret = PokemonTrade_Init(proc, state, &demo->trade, wk, 6);
    func_ov194_021bb938(demo, wk);
    PokemonTrade_SetState(wk, PokemonTradeDemoCommon_Start);
    return ret;
}

static BOOL PokemonTrade_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    u8 letter, shadow, background;
    int i;
    PokemonTradeWork *wk = work;
    BOOL done = TRUE;
    PokemonTradeState step = wk->state;
    if (func_0202d7d8()) {
        return FALSE;
    }
    if (step != NULL) {
        step(wk);
        wk->timer++;
        done = FALSE;
        for (i = 0; i < 2; i++) {
            func_ov194_021bc6b4(wk, i, wk->unk11E8[i], 0);
        }
    }
    if (wk->unkF9C != 0 && wk->unk11F3 == 0) {
        func_ov194_021b81c8(wk, 1, wk->pkm[wk->unkF9C - 1]);
        wk->unkF9C = 0;
    }
    PokemonTrade_UpdateBGM(wk);
    func_ov194_021be610(wk);
    func_0204b794();
    if (wk->menu != NULL) {
        AppTaskMenu_Update(wk->menu);
    }
    if (wk->menuWin != NULL) {
        AppTaskMenuWin_Update(wk->menuWin);
    }
    if (wk->unk11FB_4) {
        if (func_02042be8(func_02040440(), TRADE_NET_CMD_UNK17, 0, NULL)) {
            wk->unk11FB_4 = 0;
        }
    }
    if (wk->tcbEx != NULL) {
        GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
        GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
        GFL_TCBExMgrUpdate(wk->tcbEx);
        GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
        func_02021a3c(wk->printQueue);
    }
    func_ov194_021be720(wk);
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(wk->camera);
    GFL_G3DSysMtxViewFlush();
    MCSSSys_Update(wk->mcssSys);
    MCSSSys_Draw(wk->mcssSys);
    func_ov194_021c1fb8(wk);
    GFL_G3DSysReqSwapBuffers();
    if (PokemonTrade_IsNetwork(wk) && wk->netSave == NULL && GFL_NetErrCheck()) {
        if (wk->unk11F7) {
            func_02011d04(0x29);
        }
        if (wk->unk0 != NULL) {
            NHttpRap_EndRequest(wk->unk0);
            NHttpRap_Destroy(wk->unk0);
            wk->unk0 = NULL;
            DWCRap_SetErrorFunc(NULL, NULL);
            DWCRapCommon_EndSubHeap();
        }
        if (func_02042b20()) {
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
    func_ov194_021c4cfc(&wk->infoIcons[0]);
    func_ov194_021c4cfc(&wk->infoIcons[1]);
    func_ov194_021c4cfc(&wk->infoIcons[2]);
    func_ov194_021c4b88(wk);
    func_ov194_021bb4b4(wk);
    func_0202d7dc();
    if (PokemonTrade_IsNetwork(wk)) {
        func_02040c64(TRADE_NET_CMD_BASE);
    }
    func_ov194_021c1fc0(wk);
    GFL_HeapFree(wk->pkm[0]);
    GFL_HeapFree(wk->pkm[1]);
    GFL_TCBRemove(wk->vblankTcb);
    func_ov194_021bb364(wk, param != NULL);
    func_ov194_021bb1bc(wk);
    func_ov194_021bb218(wk);
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
static void func_ov194_021bbdac(PokemonTradeWork *wk, BOOL moved) {
    if (moved) {
        int column = func_ov194_021c3bc0(wk) + 2;
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
    func_ov194_021bba4c,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c6400 = {
    func_ov194_021bba8c,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions POKEMONTRADE_PROC_FUNCTIONS = {
    PokemonTrade_ProcInitGtsNego,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c63ac = {
    func_ov194_021bb9cc,
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
    func_ov194_021bba0c,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};

const GameProcFunctions data_ov194_021c640c = {
    func_ov194_021bb868,
    PokemonTrade_ProcMain,
    PokemonTrade_ProcExit,
};
