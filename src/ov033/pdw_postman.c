#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/mystery_gift_pokemon.h"
#include "field/pdw_postman.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/dream_world.h"
#include "save/high_link.h"
#include "save/mystery_gift.h"
#include "save/player_info.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/wordset.h"
#include "text/script/global_10390.h"

#define MYSTERY_GIFT_DELIVERY_MAN_OBJ_CODE 0x46

// The values of the one-shot gifts and the one-shot DR each gives
// clang-format off
static const u8 sOneShotDRGifts[8] = {
    0x33, 2,
    0x34, 3,
    0x35, 4,
    0x36, 5,
};

// clang-format on

static const PdwPostmanGiftHandler sGiftHandlers[MYSTERY_GIFT_KIND_MAX] = {
    { MYSTERY_GIFT_KIND_NONE, NULL, NULL, NULL, NULL, NULL },
    { MYSTERY_GIFT_KIND_POKEMON, doesPartyHaveSpace, ScrcmdPokemonFld_ReceiveGiftMon,
      ScrcmdPokemonFld_GetGiftMonReceiveType, ScrcmdPokemonFld_PrepareGiftMonMessage,
      ScrcmdPokemonFld_PrepareGiftMonFollowUpMessage },
    { MYSTERY_GIFT_KIND_ITEM, ScrcmdPokemonFld_CanReceiveGiftItem, ScrcmdPokemonFld_ReceiveGiftItem,
      ScrcmdPokemonFld_GetGiftItemReceiveType, ScrcmdPokemonFld_PrepareGiftItemMessage,
      ScrcmdPokemonFld_PrepareGiftItemFollowUpMessage },
    { MYSTERY_GIFT_KIND_PASS_POWER, ScrcmdPokemonFld_CanReceiveGiftFlag, ScrcmdPokemonFld_ReceiveGiftFlag,
      ScrcmdPokemonFld_GetGiftFlagReceiveType, ScrcmdPokemonFld_PrepareGiftFlagMessage,
      ScrcmdPokemonFld_PrepareGiftFlagFollowUpMessage },
    { MYSTERY_GIFT_KIND_ONE_SHOT, ScrcmdPokemonFld_CanReceiveGiftOneShot, ScrcmdPokemonFld_ReceiveGiftOneShot,
      ScrcmdPokemonFld_GetGiftOneShotReceiveType, ScrcmdPokemonFld_PrepareGiftOneShotMessage,
      ScrcmdPokemonFld_PrepareGiftOneShotFollowUpMessage },
};

PdwPostmanItemWindow *PdwPostman_CreateListWin(Field *field, PdwPostmanItem *items, u32 count) {
    u16 heapId;
    PdwPostmanItemWindow *work;
    s32 height;

    heapId = Field_GetHeapID(field);
    work = GFL_HeapAllocate(heapId, sizeof(PdwPostmanItemWindow), TRUE, "pdw_postman.c", 0x73);
    work->heapId = heapId;
    work->field = field;
    work->items = items;
    work->count = count;
    work->messages = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10390, heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    work->first = GFL_StrBufCreate(0x80, heapId);
    work->second = GFL_StrBufCreate(0x80, heapId);
    height = ((14 * (s32)count + 7) & ~7) / 8;
    work->window = FieldMsgBG_CreateMoneyWin(Field_GetMsgBGSys(field), work->messages, 1, 1, 0x15, height);
    return work;
}

void PdwPostman_DeleteListWin(PdwPostmanItemWindow *work) {
    MsgBGPrinter_ClearMsgWin(work->window);
    MsgBGPrinter_DeleteMsgWin(work->window);
    GFL_BGSysLoadScr(1);
    GFL_StrBufFree(work->first);
    GFL_StrBufFree(work->second);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_MsgDataFree(work->messages);
    GFL_HeapFree(work);
}

void PdwPostman_PrintListWin(PdwPostmanItemWindow *work) {
    MsgData *messages;
    s32 i;

    messages = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, HEAPID_TAIL(work->heapId));
    for (i = 0; i < work->count; i++) {
        GFL_MsgDataLoadStrbuf(messages, work->items[i].item, work->first);
        MsgBGPrinter_MsgWinPrintStr(work->window, 0, 14 * i, work->first);
        GFL_MsgDataLoadStrbuf(work->messages, Global10390_Text_X, work->first);
        WordSetNumber(work->wordSet, 0, work->items[i].quantity, 2, 1, 1);
        GFL_WordSetFormatStrbuf(work->wordSet, work->second, work->first);
        MsgBGPrinter_MsgWinPrintStr(work->window, 0x6c, 14 * i, work->second);
    }
    GFL_MsgDataFree(messages);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(MsgBGPrinter_GetMsgWinBmpWin(work->window)));
}

// The steps of the list: print a page of up to 7 items, wait for it to be shown and for a key, close it, and go on to
// the next page
enum {
    PDW_POSTMAN_LIST_STEP_OPEN,
    PDW_POSTMAN_LIST_STEP_WAIT_PRINT,
    PDW_POSTMAN_LIST_STEP_WAIT_KEY,
    PDW_POSTMAN_LIST_STEP_CLOSE,
    PDW_POSTMAN_LIST_STEP_NEXT,
};

GameEventReturnCode PdwPostman_ListEvent(GameEvent *unused, u32 *state, void *data) {
    PdwPostmanItemEvent *event;
    BmpWin *bitmapWindow;
    u16 remaining;

    event = data;
    switch (*state) {
    case PDW_POSTMAN_LIST_STEP_OPEN:
        remaining = event->count - event->index;
        if (remaining > 7) {
            remaining = 7;
        }
        event->window = PdwPostman_CreateListWin(event->field, &event->items[event->index], remaining);
        PdwPostman_PrintListWin(event->window);
        event->window->cursor = MsgBGPrinter_CreateCursor(Field_GetMsgBGSys(event->field));
        (*state)++;
        break;
    case PDW_POSTMAN_LIST_STEP_WAIT_PRINT:
        if (MsgBGPrinter_IsMsgWinTransDone(event->window->window) != TRUE) {
            break;
        }
        (*state)++;
        break;
    case PDW_POSTMAN_LIST_STEP_WAIT_KEY:
        bitmapWindow = MsgBGPrinter_GetMsgWinBmpWin(event->window->window);
        MsgBGPrinter_AnimateCursor(event->window->cursor, BmpWin_GetBitmap(bitmapWindow), 15);
        BmpWin_FlushChar(bitmapWindow);
        if (!(GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        (*state)++;
        break;
    case PDW_POSTMAN_LIST_STEP_CLOSE:
        MsgBGPrinter_DeleteCursor(event->window->cursor);
        PdwPostman_DeleteListWin(event->window);
        (*state)++;
        break;
    case PDW_POSTMAN_LIST_STEP_NEXT:
        if (event->index + 7 >= event->count) {
            GFL_HeapFree(event->items);
            return TRUE;
        }
        event->index += 7;
        *state = PDW_POSTMAN_LIST_STEP_OPEN;
        break;
    }
    return FALSE;
}

PdwPostmanItem *PdwPostman_CreateReceivableList(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld,
                                                u32 *count) {
    PdwPostmanItem *result;
    BagSave *bag;
    u32 n;
    s32 i;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    result = GFL_HeapAllocate(heapId, sizeof(PdwPostmanItem) * 20, TRUE, "pdw_postman.c", 0x14d);
    i = 0;
    n = 0;
    for (; i < 20; i++) {
        item = DreamWorldSave_GetItem(dreamWorld, i);
        quantity = DreamWorldSave_GetItemCount(dreamWorld, i);
        if (item != 0 && BagSave_CheckAvailItemSpace(bag, item, quantity, heapId) == TRUE) {
            result[n].item = item;
            result[n].quantity = quantity;
            n++;
        }
    }
    *count = n;
    return result;
}

void PdwPostman_ReceiveItems(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld) {
    BagSave *bag;
    u32 i;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    for (i = 0; i < 20; i++) {
        item = DreamWorldSave_GetItem(dreamWorld, i);
        quantity = DreamWorldSave_GetItemCount(dreamWorld, i);
        if (item != 0 && BagSave_AddItem(bag, item, quantity, heapId) == TRUE) {
            DreamWorldSave_ClearItem(dreamWorld, i);
        }
    }
}

u32 PdwPostman_CountUnreceivable(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld) {
    BagSave *bag;
    u32 i;
    u32 count;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    i = 0;
    count = 0;
    for (; i < 20; i++) {
        item = DreamWorldSave_GetItem(dreamWorld, i);
        quantity = DreamWorldSave_GetItemCount(dreamWorld, i);
        if (item != 0 && BagSave_CheckAvailItemSpace(bag, item, quantity, heapId) == FALSE) {
            count++;
        }
    }
    return count;
}

u16 PdwPostman_GetUnreceivableItem(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld, u32 position) {
    BagSave *bag;
    s32 i;
    u32 count;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    i = 0;
    count = 0;
    for (; i < 20; i++) {
        item = DreamWorldSave_GetItem(dreamWorld, i);
        quantity = DreamWorldSave_GetItemCount(dreamWorld, i);
        if (item != 0 && BagSave_CheckAvailItemSpace(bag, item, quantity, heapId) == FALSE) {
            if (count == position) {
                return item;
            }
            count++;
        }
    }
    return 0;
}

GameEvent *PdwPostman_CreateListEvent(GameSystem *gsys) {
    GameData *gameData;
    DreamWorldSave *dreamWorld;
    GameEvent *event;
    PdwPostmanItemEvent *work;

    gameData = GSYS_GetGameData(gsys);
    dreamWorld = getDreamWorldStuffAddress(GameData_GetSaveControl(gameData));
    event = GameEvent_Create(gsys, NULL, PdwPostman_ListEvent, sizeof(PdwPostmanItemEvent));
    work = GameEvent_GetData(event);
    work->field = GSYS_GetField(gsys);
    work->index = 0;
    work->items = PdwPostman_CreateReceivableList(gameData, Field_GetHeapID(work->field), dreamWorld, &work->count);
    return event;
}

// What Scrcmd_PdwPostman_MysteryGifts does: whether a card's gift waits, whether it can be received, set the words
// of its messages, receive it, find the delivery man, and what the gift is
enum {
    MYSTERY_GIFTS_HAS_GIFT,
    MYSTERY_GIFTS_CAN_RECEIVE,
    MYSTERY_GIFTS_PREPARE_MESSAGE,
    MYSTERY_GIFTS_PREPARE_FOLLOW_UP_MESSAGE,
    MYSTERY_GIFTS_RECEIVE,
    MYSTERY_GIFTS_GET_DELIVERY_MAN_ID,
    MYSTERY_GIFTS_IS_DELIVERY_MAN_HERE,
    MYSTERY_GIFTS_GET_RECEIVE_TYPE,
};

BOOL Scrcmd_PdwPostman_MysteryGifts(VM *vm, FieldScriptEnv *env) {
    u16 mode = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    MysteryGiftSave *save = mysteryGiftBlock(GameData_GetSaveControl(gameData), 0, heapId);
    u32 slot;
    MysteryGift buffer;
    MysteryGift *gift = PdwPostman_FindMysteryCard(save, &slot, &buffer);
    u8 kind = PdwPostman_GetMysteryCardKind(save);

    if (kind != sGiftHandlers[kind].kind) {
        MysteryGiftSave_Free(save);
        return FALSE;
    }
    switch (mode) {
    case MYSTERY_GIFTS_HAS_GIFT:
        if (kind != MYSTERY_GIFT_KIND_NONE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MYSTERY_GIFTS_CAN_RECEIVE:
        *result = ScrcmdPokemonFld_CanReceiveGift(kind, env, gameData, gift);
        break;
    case MYSTERY_GIFTS_PREPARE_MESSAGE:
        *result = ScrcmdPokemonFld_PrepareGiftReceiveMessage(kind, gift, env);
        break;
    case MYSTERY_GIFTS_PREPARE_FOLLOW_UP_MESSAGE:
        *result = ScrcmdPokemonFld_PrepareGiftFollowUpMessage(kind, gift, env);
        break;
    case MYSTERY_GIFTS_RECEIVE:
        if (ScrcmdPokemonFld_ReceiveGift(kind, env, gameData, gift) == TRUE) {
            PdwPostman_SetCardDelivered(save, slot);
            if (ScrcmdPokemonFld_IsLibertyPassGift(gift, kind) == TRUE) {
                setOneShotDRObtained(getTrainerCardDataBlkAddress(gameData), 1, GetGameDataPlayerInfo(gameData));
            }
            if (ScrcmdPokemonFld_IsLandorusTherianGift(env, gift, kind) == TRUE) {
                setOneShotDRObtained(getTrainerCardDataBlkAddress(gameData), 7, GetGameDataPlayerInfo(gameData));
            }
        }
        *result = FALSE;
        break;
    case MYSTERY_GIFTS_GET_DELIVERY_MAN_ID:
        *result = GetActorIDOfMysteryGiftDeliveryMan(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)), gameData);
        break;
    case MYSTERY_GIFTS_IS_DELIVERY_MAN_HERE:
        *result = IsMysteryGiftDeliveryManActorAvailable(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
        break;
    case MYSTERY_GIFTS_GET_RECEIVE_TYPE:
        *result = ScrcmdPokemonFld_GetGiftReceiveType(kind, env, gameData, gift);
        break;
    }
    MysteryGiftSave_Free(save);
    return FALSE;
}

BOOL ScrcmdPokemonFld_CanReceiveGift(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    BOOL (*canReceive)(FieldScriptEnv *, GameData *, MysteryGift *) = sGiftHandlers[kind].canReceive;

    if (gift != NULL && canReceive != NULL) {
        return canReceive(env, gameData, gift);
    }
    return FALSE;
}

u32 ScrcmdPokemonFld_PrepareGiftReceiveMessage(u32 kind, MysteryGift *gift, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u32 (*getMessage)(WordSet *, MysteryGift *, FieldScriptEnv *) = sGiftHandlers[kind].prepareMessage;

    if (gift != NULL && getMessage != NULL) {
        return getMessage(wordSet, gift, env);
    }
    return 7;
}

u32 ScrcmdPokemonFld_PrepareGiftFollowUpMessage(u32 kind, MysteryGift *gift, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u32 (*getMessage)(WordSet *, MysteryGift *, FieldScriptEnv *) = sGiftHandlers[kind].prepareFollowUpMessage;

    if (gift != NULL && getMessage != NULL) {
        return getMessage(wordSet, gift, env);
    }
    return 8;
}

BOOL ScrcmdPokemonFld_ReceiveGift(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    void (*receive)(FieldScriptEnv *, GameData *, MysteryGift *) = sGiftHandlers[kind].receive;

    if (gift != NULL && receive != NULL) {
        receive(env, gameData, gift);
        return TRUE;
    }
    return FALSE;
}

u32 ScrcmdPokemonFld_GetGiftReceiveType(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    u32 (*handler)(FieldScriptEnv *, GameData *, MysteryGift *) = sGiftHandlers[kind].getReceiveType;

    if (gift != NULL && handler != NULL) {
        return handler(env, gameData, gift);
    }
    return 0;
}

u16 GetActorIDOfMysteryGiftDeliveryMan(Field *field, GameData *gameData) {
    FieldActor *actor = FindMysteryGiftDeliveryManActor(field);
    if (actor != NULL) {
        return GetActorUID(actor);
    }

    s32 npcID = FindMysteryGiftDeliveryManNPCID(gameData);
    if (npcID >= 0) {
        return npcID;
    }
    return 0;
}

u16 IsMysteryGiftDeliveryManActorAvailable(Field *field) {
    return FindMysteryGiftDeliveryManActor(field) != NULL;
}

FieldActor *FindMysteryGiftDeliveryManActor(Field *field) {
    u32 index = 0;
    FieldActor *actor;
    MMSys *actorSystem = Field_GetActorSystem(field);

    while (NextActor(actorSystem, &actor, &index) == TRUE) {
        if (FldAct_GetObjCode(actor) == MYSTERY_GIFT_DELIVERY_MAN_OBJ_CODE) {
            return actor;
        }
    }
    return NULL;
}

s32 FindMysteryGiftDeliveryManNPCID(GameData *gameData) {
    EventData *eventData;
    const ZoneNPC *npcs;
    s32 count;
    s32 i;

    eventData = GameData_GetEventData(gameData);
    npcs = GetZoneNPCs(eventData);
    count = GetZoneNPCsCount(eventData);

    if (npcs == NULL || count == 0) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        if (npcs[i].modelId == MYSTERY_GIFT_DELIVERY_MAN_OBJ_CODE) {
            return npcs[i].uid;
        }
    }
    return -1;
}

BOOL ScrcmdPokemonFld_IsLibertyPassGift(MysteryGift *gift, u8 kind) {
    if (kind != MYSTERY_GIFT_KIND_ITEM) {
        return FALSE;
    }
    if (gift->id != 0x7fe) {
        return FALSE;
    }
    if (gift->value == ITEM_LIBERTY_PASS) {
        return TRUE;
    }
    return FALSE;
}

u32 ScrcmdPokemonFld_IsLandorusTherianGift(FieldScriptEnv *env, MysteryGift *gift, u8 kind) {
    PartyPkm *pkm;
    PlayerInfo *playerInfo;
    u32 result;

    if (kind == MYSTERY_GIFT_KIND_POKEMON) {
        pkm = ScrcmdPokemonFld_CreateGiftMon(env, gift);
        if (pkm != NULL) {
            playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
            result = PokeParty_IsSpecialTransfer(pkm, 8, playerInfo);
            GFL_HeapFree(pkm);
            return result;
        }
    }
    return 0;
}

PartyPkm *ScrcmdPokemonFld_CreateGiftMon(FieldScriptEnv *env, MysteryGift *gift) {
    HeapID heapId;
    GameData *gameData;

    heapId = FieldScriptEnv_GetHeapID(env);
    gameData = FieldScriptEnv_GetGameData(env);
    return CreateMysteryGiftPokemon(gift, heapId, gameData);
}

BOOL doesPartyHaveSpace(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return PokeParty_GetPkmCount(GameData_GetParty(gameData)) < 6;
}

void ScrcmdPokemonFld_ReceiveGiftMon(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    PokeParty *party;
    PartyPkm *pkm;

    party = GameData_GetParty(gameData);
    pkm = ScrcmdPokemonFld_CreateGiftMon(env, gift);
    if (PokeParty_GetPkmCount(party) < 6 && pkm != NULL) {
        PokeParty_AddPkm(party, pkm);
        addPkmToDex(GameData_GetPokedex(gameData), pkm);
        GFL_HeapFree(pkm);
    }
}

u32 ScrcmdPokemonFld_GetGiftMonReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    PartyPkm *pkm;
    u32 isEgg;

    pkm = ScrcmdPokemonFld_CreateGiftMon(env, gift);
    if (pkm == NULL) {
        return 1;
    }
    isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    GFL_HeapFree(pkm);
    if (isEgg == TRUE) {
        return 2;
    }
    return 1;
}

u32 ScrcmdPokemonFld_PrepareGiftMonMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    PartyPkm *pkm;
    u32 result;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    pkm = ScrcmdPokemonFld_CreateGiftMon(env, gift);
    result = 5;
    copyVarForText(wordSet, 0, playerInfo);
    if (pkm == NULL) {
        loadPokemonTextNameToStrbuf(wordSet, 1, 0);
    } else {
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
            result = 11;
        } else {
            loadPokemonSpeciesTextNameToStrbuf(wordSet, 1, pkm);
        }
        GFL_HeapFree(pkm);
    }
    return result;
}

u32 ScrcmdPokemonFld_PrepareGiftMonFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    return 6;
}

BOOL ScrcmdPokemonFld_CanReceiveGiftItem(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return TRUE;
}

void ScrcmdPokemonFld_ReceiveGiftItem(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    HeapID heapId;
    BagSave *bag;
    u16 item;

    heapId = FieldScriptEnv_GetHeapID(env);
    bag = GameData_GetBag(gameData);
    item = gift->value;
    if (item != ITEM_NONE && item <= ITEM_LAST) {
        BagSave_AddItem(bag, item, 1, heapId);
    }
}

u32 ScrcmdPokemonFld_GetGiftItemReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    if (ScrcmdPokemonFld_IsLibertyPassGift(gift, gift->kind) == TRUE) {
        return 4;
    }
    return 3;
}

u32 ScrcmdPokemonFld_PrepareGiftItemMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    u16 item;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    item = gift->value;
    copyVarForText(wordSet, 0, playerInfo);
    loadItemNameToStrbuf(wordSet, 1, item);
    return 7;
}

u32 ScrcmdPokemonFld_PrepareGiftItemFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    loadItemNameToStrbuf(wordSet, 0, (u16)gift->value);
    return 8;
}

BOOL ScrcmdPokemonFld_CanReceiveGiftFlag(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return TRUE;
}

void ScrcmdPokemonFld_ReceiveGiftFlag(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    HighLinkSave_SetSpecialPowerFlag(getHighLinkBlockAddress(GameData_GetSaveControl(gameData)), gift->value);
}

u32 ScrcmdPokemonFld_GetGiftFlagReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return 5;
}

u32 ScrcmdPokemonFld_PrepareGiftFlagMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    GameData *gameData;
    PlayerInfo *playerInfo;
    u32 passPower;

    gameData = FieldScriptEnv_GetGameData(env);
    getHighLinkBlockAddress(GameData_GetSaveControl(gameData));
    playerInfo = GetGameDataPlayerInfo(gameData);
    passPower = gift->value;
    copyVarForText(wordSet, 0, playerInfo);
    loadPassPowerToStrbuf(wordSet, 1, passPower);
    return 9;
}

u32 ScrcmdPokemonFld_PrepareGiftFlagFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    return 10;
}

u8 ScrcmdPokemonFld_GetGiftOneShotPower(MysteryGift *gift) {
    s32 value;

    if (gift->kind != MYSTERY_GIFT_KIND_ONE_SHOT) {
        value = MYSTERY_GIFT_ONE_SHOT_NONE;
    } else {
        value = gift->value;
        if (value < 0x33 || value > 0x36) {
            value = MYSTERY_GIFT_ONE_SHOT_NONE;
        }
    }
    return value;
}

BOOL ScrcmdPokemonFld_CanReceiveGiftOneShot(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return TRUE;
}

void ScrcmdPokemonFld_ReceiveGiftOneShot(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    PlayerInfo *playerInfo;
    TrainerCardSave *card;
    u32 value;
    unsigned int i;

    playerInfo = GetGameDataPlayerInfo(gameData);
    card = getTrainerCardDataBlkAddress(gameData);
    value = ScrcmdPokemonFld_GetGiftOneShotPower(gift);
    for (i = 0; i < 8; i += 2) {
        if (sOneShotDRGifts[i] == value) {
            setOneShotDRObtained(card, sOneShotDRGifts[i + 1], playerInfo);
            return;
        }
    }
}

u32 ScrcmdPokemonFld_GetGiftOneShotReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return 6;
}

u32 ScrcmdPokemonFld_PrepareGiftOneShotMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    u8 value;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    FieldScriptEnv_GetHeapID(env);
    value = ScrcmdPokemonFld_GetGiftOneShotPower(gift);
    copyVarForText(wordSet, 0, playerInfo);
    WordSet_LoadFestivalMission(wordSet, 1, value, 0);
    return 12;
}

u32 ScrcmdPokemonFld_PrepareGiftOneShotFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    u32 value;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    value = ScrcmdPokemonFld_GetGiftOneShotPower(gift);
    copyVarForText(wordSet, 0, playerInfo);
    loadPassPowerToStrbuf(wordSet, 1, value);
    return 13;
}

MysteryGift *PdwPostman_GetMysteryCard(MysteryGiftSave *save, u32 slot, MysteryGift *gift) {
    if (!MysteryGiftSave_IsSlotUsed(save, slot)) {
        return NULL;
    }
    if (MysteryGiftSave_IsCardDelivered(save, slot) == TRUE) {
        return NULL;
    }
    if (!MysteryGiftSave_GetCard(save, slot, gift)) {
        return NULL;
    }
    if (gift->kind == MYSTERY_GIFT_KIND_NONE) {
        return NULL;
    }
    if (gift->kind >= MYSTERY_GIFT_KIND_MAX) {
        gift = NULL;
    }
    return gift;
}

MysteryGift *PdwPostman_FindMysteryCard(MysteryGiftSave *save, u32 *slot, MysteryGift *gift) {
    s32 i;

    for (i = 0; i < 12; i++) {
        if (PdwPostman_GetMysteryCard(save, i, gift) != NULL) {
            *slot = i;
            return gift;
        }
    }
    return NULL;
}

void PdwPostman_SetCardDelivered(MysteryGiftSave *save, u32 slot) {
    MysteryGiftSave_SetCardDelivered(save, slot);
}

u8 PdwPostman_GetMysteryCardKind(MysteryGiftSave *save) {
    u32 slot;
    MysteryGift gift;
    MysteryGift *result;

    result = PdwPostman_FindMysteryCard(save, &slot, &gift);
    if (result == NULL) {
        return 0;
    }
    return result->kind;
}
