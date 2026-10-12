#include "app/pokelist.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/bmp_winframe.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// Using an item from the bag on a Pokémon of the party list, and the stats shown after a level up. The ROM doesn't
// name this file; it is named for what it does

// What an item does to a Pokémon, from its data: 0 nothing the list uses, 1 revive, 2 level up, 3 to 8 cure one
// status condition (sleep, poison, burn, freeze, paralysis, confusion), 9 cure them all, 10 restore HP, 11 restore
// HP and cure, 12 restore all HP, 13 to 18 raise an effort value, 19 to 24 lower one, 25 restore PP, 26 restore one
// move's PP, 27 restore all moves' PP, 28 raise a move's PP, 29 not usable
static u32 PokeList_GetItemEffect(u16 item);
static BOOL PokeList_HasEffort(PartyPkm *pkm, u32 param, u16 item);
static void PokeList_ShowItemMessage(PokeListWork *wk, u32 offset, void (*doneFunc)(PokeListWork *wk));
static void PokeList_ItemHealDone(PokeListWork *wk);
static void PokeList_ItemReviveDone(PokeListWork *wk);
static void PokeList_ItemReviveNext(PokeListWork *wk);
static void PokeList_ShowPkmMessage(PokeListWork *wk, u32 msgId);
static void PokeList_ShowStatMessage(PokeListWork *wk, u32 msgId, u16 stat);
static void PokeList_LevelUpMessageDone(PokeListWork *wk);

// The stats in the order the level up shows them
static const u16 sStatParams[] = {
    PKM_PARAM_MAX_HP, PKM_PARAM_ATTACK, PKM_PARAM_DEFENSE, PKM_PARAM_SP_ATTACK, PKM_PARAM_SP_DEFENSE, PKM_PARAM_SPEED,
};

static u32 PokeList_GetItemEffect(u16 item) {
    void *data = PML_ItemReadDataFile(item, 0, HEAPID_POKELIST);
    s32 cures;
    s32 effort;

    if (PML_ItemGetParam(data, 14) != 1) {
        GFL_HeapFree(data);
        return 29;
    }
    if (PML_ItemGetParam(data, 30) != 0 || PML_ItemGetParam(data, 31) != 0 || PML_ItemGetParam(data, 32) != 0 ||
        PML_ItemGetParam(data, 33) != 0 || PML_ItemGetParam(data, 34) != 0 || PML_ItemGetParam(data, 35) != 0 ||
        PML_ItemGetParam(data, 36) != 0) {
        GFL_HeapFree(data);
        return 0;
    }
    if (PML_ItemGetParam(data, 27) != 0) {
        GFL_HeapFree(data);
        return 1;
    }
    if (PML_ItemGetParam(data, 28) != 0) {
        GFL_HeapFree(data);
        return 2;
    }
    cures = PML_ItemGetParam(data, 18);
    cures += PML_ItemGetParam(data, 19) << 1;
    cures += PML_ItemGetParam(data, 20) << 2;
    cures += PML_ItemGetParam(data, 21) << 3;
    cures += PML_ItemGetParam(data, 22) << 4;
    cures += PML_ItemGetParam(data, 23) << 5;
    switch (cures) {
    case 0x1:
        GFL_HeapFree(data);
        return 3;
    case 0x2:
        GFL_HeapFree(data);
        return 4;
    case 0x4:
        GFL_HeapFree(data);
        return 5;
    case 0x8:
        GFL_HeapFree(data);
        return 6;
    case 0x10:
        GFL_HeapFree(data);
        return 7;
    case 0x20:
        GFL_HeapFree(data);
        return 8;
    case 0x3f:
        if (PML_ItemGetParam(data, 41) != 0) {
            GFL_HeapFree(data);
            return 11;
        }
        GFL_HeapFree(data);
        return 9;
    }
    if (PML_ItemGetParam(data, 24) != 0) {
        GFL_HeapFree(data);
        return 10;
    }
    if (PML_ItemGetParam(data, 41) != 0) {
        GFL_HeapFree(data);
        return 11;
    }
    if (PML_ItemGetParam(data, 26) != 0) {
        GFL_HeapFree(data);
        return 12;
    }
    effort = PML_ItemGetParam(data, 52);
    if (effort > 0) {
        GFL_HeapFree(data);
        return 13;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return 19;
    }
    effort = PML_ItemGetParam(data, 53);
    if (effort > 0) {
        GFL_HeapFree(data);
        return 14;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return 20;
    }
    effort = PML_ItemGetParam(data, 54);
    if (effort > 0) {
        GFL_HeapFree(data);
        return 15;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return 21;
    }
    effort = PML_ItemGetParam(data, 55);
    if (effort > 0) {
        GFL_HeapFree(data);
        return 16;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return 22;
    }
    effort = PML_ItemGetParam(data, 56);
    if (effort > 0) {
        GFL_HeapFree(data);
        return 17;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return 23;
    }
    effort = PML_ItemGetParam(data, 57);
    if (effort > 0) {
        GFL_HeapFree(data);
        return 18;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return 24;
    }
    if (PML_ItemGetParam(data, 29) != 0) {
        GFL_HeapFree(data);
        return 25;
    }
    if (PML_ItemGetParam(data, 37) != 0) {
        GFL_HeapFree(data);
        return 26;
    }
    if (PML_ItemGetParam(data, 38) != 0) {
        GFL_HeapFree(data);
        return 27;
    }
    if (PML_ItemGetParam(data, 39) != 0 || PML_ItemGetParam(data, 40) != 0) {
        GFL_HeapFree(data);
        return 28;
    }
    GFL_HeapFree(data);
    return 29;
}

// Whether the item is used on one of the Pokémon's moves, which the menu picks
BOOL PokeList_IsItemForMove(PokeListWork *wk, u16 item) {
    switch (PokeList_GetItemEffect(item)) {
    case 26:
        return TRUE;
    case 27:
        return TRUE;
    case 28:
        if (GetItemParam(item, 39, wk->heapId)) {
            return TRUE;
        }
    }
    return FALSE;
}

// Whether the item is used on the whole party at once, as Sacred Ash is
BOOL PokeList_IsItemForParty(PokeListWork *wk, u16 item) {
    if (GetItemParam(item, 27, wk->heapId)) {
        return TRUE;
    }
    return FALSE;
}

// The first Pokémon of the party that the item can be used on, or -1
s32 PokeList_FindItemTarget(PokeListWork *wk) {
    u8 count = PokeParty_GetPkmCount(wk->param->party);
    u8 i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, i);

        if (StatusRcv_CanUseItem(pkm, wk->param->item, 0, wk->heapId) == TRUE) {
            return i;
        }
    }
    return -1;
}

// The message of the menu that picks a move for the item
u32 PokeList_GetItemMenuMessage(PokeListWork *wk, u16 item) {
    switch (PokeList_GetItemEffect(item)) {
    case 26:
    case 27:
        return 0x17;
    case 28:
        return 0x16;
    }
    return 0x16;
}

// Changes Arceus's type to the plate it holds
void PokeList_UpdateArceusForm(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_ARCEUS) {
        u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        u16 type = _getTypeForPlate(item);

        if (form != type) {
            PokeParty_ChangeForme(pkm, type);
        }
    }
}

// Changes Genesect's form to the drive it holds
void PokeList_UpdateGenesectForm(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_GENESECT) {
        u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        u32 driveForm = PML_ItemGetGenesectDriveForme(item);

        if (form != driveForm) {
            PokeParty_ChangeForme(pkm, driveForm);
        }
    }
}

// Shows message 0x52 + offset with the item's name
static void PokeList_ShowItemMessage(PokeListWork *wk, u32 offset, void (*doneFunc)(PokeListWork *wk)) {
    wk->param->result = 10;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetItemName(wk, wk->message, 0, wk->param->item);
    PokeList_ShowMessage(wk, offset + 0x52, TRUE, doneFunc);
    PokeListMessage_FreeWordSet(wk, wk->message);
}

// The item had no effect: the list ends
void PokeList_ShowItemUselessExit(PokeListWork *wk) {
    PokeList_ShowItemMessage(wk, 0, PokeList_MessageDoneExit);
}

void PokeList_ShowItemUseless(PokeListWork *wk) {
    PokeList_ShowItemMessage(wk, 0, PokeList_MessageDoneSelect);
}

void PokeList_ShowItemMessageSelect(PokeListWork *wk, u32 offset) {
    PokeList_ShowItemMessage(wk, offset, PokeList_MessageDoneSelect);
}

// Shows what the item did, and returns its effect
u32 PokeList_ShowItemResult(PokeListWork *wk, u32 move) {
    u32 effect = PokeList_GetItemEffect(wk->param->item);

    switch (effect) {
    case 1:
        wk->state = 10;
        wk->prevHp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);
        wk->hpDoneFunc = PokeList_ItemReviveDone;
        break;
    case 2: {
        u32 level = PokeParty_GetLevel(wk->pkm);
        StrBuf *name;
        u8 i;

        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_SetNumber(wk, wk->message, 1, level + 1, 3);
        PokeList_ShowMessage(wk, 0xa9, TRUE, PokeList_LevelUpMessageDone);
        PokeListMessage_FreeWordSet(wk, wk->message);
        name = GFL_StrBufCreate(12, wk->heapId);
        PokeParty_GetParam(wk->pkm, PKM_PARAM_NICKNAME, name);
        func_0202d2c8(name);
        GFL_StrBufFree(name);
        for (i = 0; i < 6; i++) {
            wk->prevStats[i] = PokeParty_GetParam(wk->pkm, sStatParams[i], NULL);
        }
        break;
    }
    case 3:
        PokeList_ShowPkmMessage(wk, 0x48);
        break;
    case 4:
        PokeList_ShowPkmMessage(wk, 0x2e);
        break;
    case 5:
        PokeList_ShowPkmMessage(wk, 0x30);
        break;
    case 6:
        PokeList_ShowPkmMessage(wk, 0x31);
        break;
    case 7:
        PokeList_ShowPkmMessage(wk, 0x2f);
        break;
    case 9:
        PokeList_ShowPkmMessage(wk, 0x34);
        break;
    case 11:
    case 12:
        wk->state = 10;
        wk->prevHp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);
        wk->hpDoneFunc = PokeList_ItemHealDone;
        break;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
        PokeList_ShowStatMessage(wk, 0x38, effect - 13);
        break;
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24: {
        u16 stat = effect - 19;

        if (PokeList_HasEffort(wk->pkm, PKM_PARAM_EV_HP + stat, wk->param->item) == FALSE) {
            PokeList_ShowStatMessage(wk, 0x5b, stat);
        } else if (PokeParty_GetParam(wk->pkm, PKM_PARAM_HAPPINESS, NULL) == 255) {
            PokeList_ShowStatMessage(wk, 0x5a, stat);
        } else {
            PokeList_ShowStatMessage(wk, 0x59, stat);
        }
        break;
    }
    case 26:
    case 27:
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetMoveName(wk, wk->message, 0, move);
        PokeList_ShowMessage(wk, 0x35, TRUE, PokeList_MessageDoneItem);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case 28:
        PokeList_ShowPkmMessage(wk, 0x32);
        break;
    case 29:
        break;
    }
    return effect;
}

static void PokeList_ItemHealDone(PokeListWork *wk) {
    u16 hp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);

    wk->param->result = 10;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    if (wk->prevHp != 0) {
        PokeListMessage_SetNumber(wk, wk->message, 1, hp - wk->prevHp, 3);
        PokeList_ShowMessage(wk, 0x2d, TRUE, PokeList_MessageDoneItem);
    } else {
        PokeList_ShowMessage(wk, 0x33, TRUE, PokeList_MessageDoneItem);
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
}

static void PokeList_ItemReviveDone(PokeListWork *wk) {
    wk->param->result = 10;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    if (PokeList_FindItemTarget(wk) != -1) {
        PokeList_ShowMessage(wk, 0x33, TRUE, PokeList_ItemReviveNext);
    } else {
        PokeList_ShowMessage(wk, 0x33, TRUE, PokeList_MessageDoneExit);
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
}

// Revives the next fainted Pokémon, for an item used on the whole party
static void PokeList_ItemReviveNext(PokeListWork *wk) {
    s32 pos = PokeList_FindItemTarget(wk);

    PokeListMessage_Close(wk, wk->message);
    PokeListPlate_SetPalette(wk, wk->plates[wk->cursorPos], 3);
    wk->cursorPos = pos;
    wk->pkm = PokeParty_GetPkm(wk->param->party, pos);
    PokeList_ShowItemResult(wk, 0);
    StatusRcv_UseItem(wk->pkm, wk->param->item, 0, wk->param->zoneId, wk->heapId);
    PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
    GFL_SndSEPlay(SEQ_SE_RECOVERY);
}

static void PokeList_LevelUpMessageDone(PokeListWork *wk) {
    wk->state = 13;
    wk->subState = 0;
    wk->param->learnIndex = 0;
    PokeListMessage_DrawKeyCursor(wk, wk->message);
}

// Called once the message of a move learned is read: on to the next move
void PokeList_LearnMessageDone(PokeListWork *wk) {
    wk->state = 13;
    wk->subState = 6;
}

// The level up: shows the stats' gains, then the stats, then teaches the moves learned at the level, and checks for
// an evolution
void PokeList_UpdateLevelUp(PokeListWork *wk) {
    switch (wk->subState) {
    case 0: {
        BOOL bigGain = FALSE;
        u8 i;
        u8 row;

        wk->statsWindow = BmpWin_CreateDynamic(0, 1, 1, 14, 12, 14, TRUE);
        BmpWin_DrawFrame(wk->statsWindow, WINFRAME_TRANSFER_VBLANK, 1, 12);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->statsWindow), 15);
        BmpWin_Transfer(wk->statsWindow);
        for (i = 0; i < 6; i++) {
            u32 gain = PokeParty_GetParam(wk->pkm, sStatParams[i], NULL) - wk->prevStats[i];
            StrBuf *name;
            StrBuf *format;
            StrBuf *str;
            WordSet *wordSet;
            u16 width;

            if (gain >= 10) {
                bigGain = TRUE;
            }
            name = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xa1 + i);
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), 0, i * 16, name, wk->font, 0x440);
            GFL_StrBufFree(name);
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xa8);
            str = GFL_StrBufCreate(32, wk->heapId);
            wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
            WordSetNumber(wordSet, 0, gain, 3, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, format);
            width = GFL_FontGetBlockWidth(str, wk->font, 0);
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), 104 - width, i * 16, str, wk->font, 0x440);
            GFL_StrBufFree(format);
            GFL_StrBufFree(str);
            GFL_WordSetSystemFree(wordSet);
        }
        for (row = 0; row < 6; row++) {
            u8 x = bigGain == TRUE ? 72 : 80;
            StrBuf *plus = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xa7);

            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), x, row * 16, plus, wk->font, 0x440);
            GFL_StrBufFree(plus);
        }
        wk->subState = 1;
        break;
    }
    case 1:
        if (func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow)) == FALSE) {
            BmpWin_Transfer(wk->statsWindow);
            wk->subState = 2;
        }
        break;
    case 2:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) ||
            func_0203da48() == TRUE) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            wk->subState = 3;
        }
        break;
    case 3: {
        u8 i;

        GFL_BitmapFillArea(BmpWin_GetBitmap(wk->statsWindow), 80, 0, 32, 96, 15);
        for (i = 0; i < 6; i++) {
            u32 stat = PokeParty_GetParam(wk->pkm, sStatParams[i], NULL);
            StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xa8);
            StrBuf *str = GFL_StrBufCreate(32, wk->heapId);
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
            u16 width;

            WordSetNumber(wordSet, 0, stat, 3, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, format);
            width = GFL_FontGetBlockWidth(str, wk->font, 0);
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), 104 - width, i * 16, str, wk->font, 0x440);
            GFL_StrBufFree(format);
            GFL_StrBufFree(str);
            GFL_WordSetSystemFree(wordSet);
        }
        wk->subState = 4;
        break;
    }
    case 4:
        if (func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow)) == FALSE) {
            BmpWin_Transfer(wk->statsWindow);
            wk->subState = 5;
        }
        break;
    case 5:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) ||
            func_0203da48() == TRUE) {
            wk->subState = 6;
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            BmpWin_ClearScreen(wk->statsWindow);
            BmpWin_ClearFrame(wk->statsWindow, WINFRAME_TRANSFER_VBLANK);
            BmpWin_Free(wk->statsWindow);
            wk->statsWindow = NULL;
        }
        break;
    case 6: {
        u16 move = PokeParty_LearnNextLevelMove(wk->pkm, &wk->param->learnIndex, wk->heapId);

        PokeListMessage_Close(wk, wk->message);
        if (move == 0) {
            wk->subState = 7;
        } else if (move == LEARN_MOVE_KNOWN) {
            wk->subState = 7;
        } else if (move & LEARN_MOVE_NO_SLOT) {
            wk->param->move = move - LEARN_MOVE_NO_SLOT;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, 0x21, FALSE, PokeList_AskStopLearning);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, move);
            PokeList_ShowMessage(wk, 0x2a, TRUE, PokeList_LearnMessageDone);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    }
    case 7: {
        // Read but not used
        u32 species = PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL);

        if (CheckEvolveSpecies(wk->param->party, wk->pkm, 0, wk->param->zoneId, wk->param->season, NULL, wk->heapId) !=
            0) {
            wk->state = 19;
            wk->param->index = wk->cursorPos;
            wk->param->result = 9;
        } else {
            wk->param->mode = 5;
            wk->param->index = wk->cursorPos;
            wk->param->result = 10;
            PokeList_MessageDoneItem(wk);
        }
        break;
    }
    }
}

static BOOL PokeList_HasEffort(PartyPkm *pkm, u32 param, u16 item) {
    BOOL result = FALSE;

    if (PokeParty_GetParam(pkm, param, NULL) != 0) {
        result = TRUE;
    }
    return result;
}

static void PokeList_ShowPkmMessage(PokeListWork *wk, u32 msgId) {
    wk->param->result = 10;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    PokeList_ShowMessage(wk, msgId, TRUE, PokeList_MessageDoneItem);
    PokeListMessage_FreeWordSet(wk, wk->message);
}

static void PokeList_ShowStatMessage(PokeListWork *wk, u32 msgId, u16 stat) {
    wk->param->result = 10;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    PokeListMessage_SetStatName(wk, wk->message, 1, stat);
    PokeList_ShowMessage(wk, msgId, TRUE, PokeList_MessageDoneItem);
    PokeListMessage_FreeWordSet(wk, wk->message);
}
