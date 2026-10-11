#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/mystery_gift_pokemon.h"
#include "field/pdw_postman.h"
#include "field/field_script.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/dream_world.h"
#include "save/high_link.h"
#include "save/mystery_gift.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/wordset.h"
#include "text/script/global_10390.h"

#define MYSTERY_GIFT_DELIVERY_MAN_OBJ_CODE 0x46

// The values of the gifts of kind 4 and the one-shot DR each gives
static const u8 sOneShotDRGifts[8] = {
    0x33, 2,
    0x34, 3,
    0x35, 4,
    0x36, 5,
};

static const PdwPostmanGiftHandler sGiftHandlers[5] = {
    { 0, NULL, NULL, NULL, NULL, NULL },
    { 1, doesPartyHaveSpace, func_ov033_02178110, func_ov033_02178154, func_ov033_02178180, func_ov033_021781e0 },
    { 2, func_ov033_021781e4, func_ov033_021781e8, func_ov033_02178218, func_ov033_02178230, func_ov033_02178260 },
    { 3, func_ov033_02178274, func_ov033_02178278, func_ov033_02178290, func_ov033_02178294, func_ov033_021782cc },
    { 4, func_ov033_021782f0, func_ov033_021782f4, func_ov033_02178334, func_ov033_02178338, func_ov033_02178374 },
};

PdwPostmanItemWindow *func_ov033_02177998(Field *field, PdwPostmanItem *items, u32 count) {
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

void func_ov033_02177a28(PdwPostmanItemWindow *work) {
    func_ov036_02187c7c(work->window);
    func_ov036_02187c1c(work->window);
    GFL_BGSysLoadScr(1);
    GFL_StrBufFree(work->first);
    GFL_StrBufFree(work->second);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_MsgDataFree(work->messages);
    GFL_HeapFree(work);
}

void func_ov033_02177a60(PdwPostmanItemWindow *work) {
    MsgData *messages;
    s32 i;

    messages = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, HEAPID_TAIL(work->heapId));
    for (i = 0; i < work->count; i++) {
        GFL_MsgDataLoadStrbuf(messages, work->items[i].item, work->first);
        func_ov036_02187c4c(work->window, 0, 14 * i, work->first);
        GFL_MsgDataLoadStrbuf(work->messages, Global10390_Text_X, work->first);
        WordSetNumber(work->wordSet, 0, work->items[i].quantity, 2, 1, 1);
        GFL_WordSetFormatStrbuf(work->wordSet, work->second, work->first);
        func_ov036_02187c4c(work->window, 0x6c, 14 * i, work->second);
    }
    GFL_MsgDataFree(messages);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(func_ov036_02187c9c(work->window)));
}

GameEventReturnCode func_ov033_02177b08(GameEvent *unused, u32 *state, void *data) {
    PdwPostmanItemEvent *event;
    BmpWin *bitmapWindow;
    u16 remaining;

    event = data;
    switch (*state) {
    case 0:
        remaining = event->count - event->index;
        if (remaining > 7) {
            remaining = 7;
        }
        event->window = func_ov033_02177998(event->field, &event->items[event->index], remaining);
        func_ov033_02177a60(event->window);
        event->window->unk10 = func_ov036_02189cb0(Field_GetMsgBGSys(event->field));
        (*state)++;
        break;
    case 1:
        if (func_ov036_02187c70(event->window->window) != TRUE) {
            break;
        }
        (*state)++;
        break;
    case 2:
        bitmapWindow = func_ov036_02187c9c(event->window->window);
        func_ov036_02189de8(event->window->unk10, BmpWin_GetBitmap(bitmapWindow), 15);
        BmpWin_FlushChar(bitmapWindow);
        if (!(GCTX_HIDGetPressedKeys() & 3)) {
            break;
        }
        GFL_SndSEPlay(0x547);
        (*state)++;
        break;
    case 3:
        func_ov036_02189cd8(event->window->unk10);
        func_ov033_02177a28(event->window);
        (*state)++;
        break;
    case 4:
        if (event->index + 7 >= event->count) {
            GFL_HeapFree(event->items);
            return TRUE;
        }
        event->index += 7;
        *state = 0;
        break;
    }
    return FALSE;
}

PdwPostmanItem *func_ov033_02177bd4(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld, u32 *count) {
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
        item = func_02009a18(dreamWorld, i);
        quantity = func_02009a38(dreamWorld, i);
        if (item != 0 && BagSave_CheckAvailItemSpace(bag, item, quantity, heapId) == TRUE) {
            result[n].item = item;
            result[n].quantity = quantity;
            n++;
        }
    }
    *count = n;
    return result;
}

void func_ov033_02177c48(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld) {
    BagSave *bag;
    u32 i;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    for (i = 0; i < 20; i++) {
        item = func_02009a18(dreamWorld, i);
        quantity = func_02009a38(dreamWorld, i);
        if (item != 0 && BagSave_AddItem(bag, item, quantity, heapId) == TRUE) {
            func_02009a6c(dreamWorld, i);
        }
    }
}

u32 func_ov033_02177c8c(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld) {
    BagSave *bag;
    u32 i;
    u32 count;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    i = 0;
    count = 0;
    for (; i < 20; i++) {
        item = func_02009a18(dreamWorld, i);
        quantity = func_02009a38(dreamWorld, i);
        if (item != 0 && BagSave_CheckAvailItemSpace(bag, item, quantity, heapId) == FALSE) {
            count++;
        }
    }
    return count;
}

u16 func_ov033_02177cd4(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld, u32 position) {
    BagSave *bag;
    s32 i;
    u32 count;
    u16 item;
    u16 quantity;

    bag = GameData_GetBag(gameData);
    i = 0;
    count = 0;
    for (; i < 20; i++) {
        item = func_02009a18(dreamWorld, i);
        quantity = func_02009a38(dreamWorld, i);
        if (item != 0 && BagSave_CheckAvailItemSpace(bag, item, quantity, heapId) == FALSE) {
            if (count == position) {
                return item;
            }
            count++;
        }
    }
    return 0;
}

GameEvent *func_ov033_02177d28(GameSystem *gsys) {
    GameData *gameData;
    DreamWorldSave *dreamWorld;
    GameEvent *event;
    PdwPostmanItemEvent *work;

    gameData = GSYS_GetGameData(gsys);
    dreamWorld = getDreamWorldStuffAddress(GameData_GetSaveControl(gameData));
    event = GameEvent_Create(gsys, NULL, func_ov033_02177b08, sizeof(PdwPostmanItemEvent));
    work = GameEvent_GetData(event);
    work->field = GSYS_GetField(gsys);
    work->index = 0;
    work->items = func_ov033_02177bd4(gameData, Field_GetHeapID(work->field), dreamWorld, &work->count);
    return event;
}

BOOL func_ov033_02177d78(VM *vm, FieldScriptEnv *env) {
    u16 mode = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    MysteryGiftSave *save = mysteryGiftBlock(GameData_GetSaveControl(gameData), 0, heapId);
    u32 slot;
    MysteryGift buffer;
    MysteryGift *gift = func_ov033_021783f8(save, &slot, &buffer);
    u8 kind = func_ov033_02178428(save);

    if (kind != sGiftHandlers[kind].kind) {
        func_0200aa54(save);
        return FALSE;
    }
    switch (mode) {
    case 0:
        if (kind != 0) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case 1:
        *result = func_ov033_02177ed0(kind, env, gameData, gift);
        break;
    case 2:
        *result = func_ov033_02177ef4(kind, gift, env);
        break;
    case 3:
        *result = func_ov033_02177f28(kind, gift, env);
        break;
    case 4:
        if (func_ov033_02177f5c(kind, env, gameData, gift) == TRUE) {
            func_ov033_02178420(save, slot);
            if (func_ov033_02178074(gift, kind) == TRUE) {
                setOneShotDRObtained(getTrainerCardDataBlkAddress(gameData), 1, GetGameDataPlayerInfo(gameData));
            }
            if (func_ov033_021780a4(env, gift, kind) == TRUE) {
                setOneShotDRObtained(getTrainerCardDataBlkAddress(gameData), 7, GetGameDataPlayerInfo(gameData));
            }
        }
        *result = FALSE;
        break;
    case 5:
        *result = GetActorIDOfMysteryGiftDeliveryMan(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)), gameData);
        break;
    case 6:
        *result = IsMysteryGiftDeliveryManActorAvailable(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
        break;
    case 7:
        *result = func_ov033_02177f84(kind, env, gameData, gift);
        break;
    }
    func_0200aa54(save);
    return FALSE;
}

BOOL func_ov033_02177ed0(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    BOOL (*canReceive)(FieldScriptEnv *, GameData *, MysteryGift *) = sGiftHandlers[kind].canReceive;

    if (gift != NULL && canReceive != NULL) {
        return canReceive(env, gameData, gift);
    }
    return FALSE;
}

u32 func_ov033_02177ef4(u32 kind, MysteryGift *gift, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u32 (*getMessage)(WordSet *, MysteryGift *, FieldScriptEnv *) = sGiftHandlers[kind].unk10;

    if (gift != NULL && getMessage != NULL) {
        return getMessage(wordSet, gift, env);
    }
    return 7;
}

u32 func_ov033_02177f28(u32 kind, MysteryGift *gift, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u32 (*getMessage)(WordSet *, MysteryGift *, FieldScriptEnv *) = sGiftHandlers[kind].unk14;

    if (gift != NULL && getMessage != NULL) {
        return getMessage(wordSet, gift, env);
    }
    return 8;
}

BOOL func_ov033_02177f5c(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    void (*receive)(FieldScriptEnv *, GameData *, MysteryGift *) = sGiftHandlers[kind].receive;

    if (gift != NULL && receive != NULL) {
        receive(env, gameData, gift);
        return TRUE;
    }
    return FALSE;
}

u32 func_ov033_02177f84(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    u32 (*handler)(FieldScriptEnv *, GameData *, MysteryGift *) = sGiftHandlers[kind].unk0C;

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

BOOL func_ov033_02178074(MysteryGift *gift, u32 kind) {
    if (kind != 2) {
        return FALSE;
    }
    if (gift->id != 0x7fe) {
        return FALSE;
    }
    if (gift->value == 0x23e) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov033_021780a4(FieldScriptEnv *env, MysteryGift *gift, u32 kind) {
    PartyPkm *pkm;
    PlayerInfo *playerInfo;
    u32 result;

    if (kind == 1) {
        pkm = func_ov033_021780d8(env, gift);
        if (pkm != NULL) {
            playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
            result = PokeParty_IsSpecialTransfer(pkm, 8, playerInfo);
            GFL_HeapFree(pkm);
            return result;
        }
    }
    return 0;
}

PartyPkm *func_ov033_021780d8(FieldScriptEnv *env, MysteryGift *gift) {
    HeapID heapId;
    GameData *gameData;

    heapId = FieldScriptEnv_GetHeapID(env);
    gameData = FieldScriptEnv_GetGameData(env);
    return func_ov012_02153160(gift, heapId, gameData);
}

BOOL doesPartyHaveSpace(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return PokeParty_GetPkmCount(GameData_GetParty(gameData)) < 6;
}

void func_ov033_02178110(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    PokeParty *party;
    PartyPkm *pkm;

    party = GameData_GetParty(gameData);
    pkm = func_ov033_021780d8(env, gift);
    if (PokeParty_GetPkmCount(party) < 6 && pkm != NULL) {
        PokeParty_AddPkm(party, pkm);
        addPkmToDex(GameData_GetPokedex(gameData), pkm);
        GFL_HeapFree(pkm);
    }
}

u32 func_ov033_02178154(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    PartyPkm *pkm;
    u32 isEgg;

    pkm = func_ov033_021780d8(env, gift);
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

u32 func_ov033_02178180(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    PartyPkm *pkm;
    u32 result;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    pkm = func_ov033_021780d8(env, gift);
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

u32 func_ov033_021781e0(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    return 6;
}

BOOL func_ov033_021781e4(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return TRUE;
}

void func_ov033_021781e8(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    HeapID heapId;
    BagSave *bag;
    u16 item;

    heapId = FieldScriptEnv_GetHeapID(env);
    bag = GameData_GetBag(gameData);
    item = gift->value;
    if (item != 0 && item <= 0x27e) {
        BagSave_AddItem(bag, item, 1, heapId);
    }
}

u32 func_ov033_02178218(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    if (func_ov033_02178074(gift, gift->kind) == TRUE) {
        return 4;
    }
    return 3;
}

u32 func_ov033_02178230(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    u16 item;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    item = gift->value;
    copyVarForText(wordSet, 0, playerInfo);
    loadItemNameToStrbuf(wordSet, 1, item);
    return 7;
}

u32 func_ov033_02178260(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    loadItemNameToStrbuf(wordSet, 0, (u16)gift->value);
    return 8;
}

BOOL func_ov033_02178274(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return TRUE;
}

void func_ov033_02178278(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    func_0200c6a0(getHighLinkBlockAddress(GameData_GetSaveControl(gameData)), gift->value);
}

u32 func_ov033_02178290(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return 5;
}

u32 func_ov033_02178294(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
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

u32 func_ov033_021782cc(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    return 10;
}

u8 func_ov033_021782d0(MysteryGift *gift) {
    s32 value;

    if (gift->kind != 4) {
        value = 0x7f;
    } else {
        value = gift->value;
        if (value < 0x33 || value > 0x36) {
            value = 0x7f;
        }
    }
    return value;
}


BOOL func_ov033_021782f0(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return TRUE;
}

void func_ov033_021782f4(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    PlayerInfo *playerInfo;
    TrainerCardSave *card;
    u32 value;
    u32 i;

    playerInfo = GetGameDataPlayerInfo(gameData);
    card = getTrainerCardDataBlkAddress(gameData);
    value = func_ov033_021782d0(gift);
    for (i = 0; i < 8; i += 2) {
        if (sOneShotDRGifts[i] == value) {
            setOneShotDRObtained(card, sOneShotDRGifts[i + 1], playerInfo);
            return;
        }
    }
}

u32 func_ov033_02178334(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift) {
    return 6;
}

u32 func_ov033_02178338(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    u8 value;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    FieldScriptEnv_GetHeapID(env);
    value = func_ov033_021782d0(gift);
    copyVarForText(wordSet, 0, playerInfo);
    func_02024868(wordSet, 1, value, 0);
    return 12;
}

u32 func_ov033_02178374(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env) {
    PlayerInfo *playerInfo;
    u32 value;

    playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    value = func_ov033_021782d0(gift);
    copyVarForText(wordSet, 0, playerInfo);
    loadPassPowerToStrbuf(wordSet, 1, value);
    return 13;
}

MysteryGift *func_ov033_021783a8(MysteryGiftSave *save, u32 slot, MysteryGift *gift) {
    if (!func_0200a800(save, slot)) {
        return NULL;
    }
    if (func_0200a820(save, slot) == TRUE) {
        return NULL;
    }
    if (!func_0200a71c(save, slot, gift)) {
        return NULL;
    }
    if (gift->kind == 0) {
        return NULL;
    }
    if (gift->kind >= 5) {
        gift = NULL;
    }
    return gift;
}

MysteryGift *func_ov033_021783f8(MysteryGiftSave *save, u32 *slot, MysteryGift *gift) {
    s32 i;

    for (i = 0; i < 12; i++) {
        if (func_ov033_021783a8(save, i, gift) != NULL) {
            *slot = i;
            return gift;
        }
    }
    return NULL;
}

void func_ov033_02178420(MysteryGiftSave *save, u32 slot) {
    func_0200a858(save, slot);
}

u8 func_ov033_02178428(MysteryGiftSave *save) {
    u32 slot;
    MysteryGift gift;
    MysteryGift *result;

    result = func_ov033_021783f8(save, &slot, &gift);
    if (result == NULL) {
        return 0;
    }
    return result->kind;
}
