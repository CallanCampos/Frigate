#ifndef POKEBW2_FIELD_PDW_POSTMAN_H
#define POKEBW2_FIELD_PDW_POSTMAN_H

// The postman of overlay 33's pdw_postman.c, who hands out the items sent from the Pokémon Dream World and the
// mystery gifts. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "save/mystery_gift.h"
#include "struct_decls.h"
#include "system/game_event.h"

// An item sent from the Dream World and the number of it
typedef struct {
    u16 item;
    u16 quantity;
} PdwPostmanItem;

// The window that lists up to 7 of the items
typedef struct {
    u16 heapId;
    Field *field;
    MsgData *messages;
    MoneyWin *window;
    MsgBGCursor *cursor;
    WordSet *wordSet;
    StrBuf *first;
    StrBuf *second;
    s32 count;
    PdwPostmanItem *items;
} PdwPostmanItemWindow;

// The event that shows the items 7 at a time
typedef struct {
    Field *field;
    PdwPostmanItemWindow *window;
    PdwPostmanItem *items;
    u32 count;
    u16 index;
} PdwPostmanItemEvent;

// What the postman does for each kind of mystery gift, in order of MysteryGift.kind
typedef struct {
    u32 kind;
    BOOL (*canReceive)(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
    void (*receive)(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
    // What the gift is to the script: 1 a Pokémon, 2 an egg, 3 an item, 4 the Liberty Pass, 5 a Pass Power, 6 a
    // one-shot power
    u32 (*getReceiveType)(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
    // These set the words of the message on receiving the gift and of the one after it, and return the message
    u32 (*prepareMessage)(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
    u32 (*prepareFollowUpMessage)(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
} PdwPostmanGiftHandler;

PdwPostmanItemWindow *PdwPostman_CreateListWin(Field *field, PdwPostmanItem *items, u32 count);
void PdwPostman_DeleteListWin(PdwPostmanItemWindow *work);
void PdwPostman_PrintListWin(PdwPostmanItemWindow *work);
GameEventReturnCode PdwPostman_ListEvent(GameEvent *event, u32 *state, void *data);
PdwPostmanItem *PdwPostman_CreateReceivableList(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld,
                                                u32 *count);
void PdwPostman_ReceiveItems(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld);
u32 PdwPostman_CountUnreceivable(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld);
u16 PdwPostman_GetUnreceivableItem(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld, u32 position);
GameEvent *PdwPostman_CreateListEvent(GameSystem *gsys);
BOOL Scrcmd_PdwPostman_MysteryGifts(VM *vm, FieldScriptEnv *env);
BOOL ScrcmdPokemonFld_CanReceiveGift(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_PrepareGiftReceiveMessage(u32 kind, MysteryGift *gift, FieldScriptEnv *env);
u32 ScrcmdPokemonFld_PrepareGiftFollowUpMessage(u32 kind, MysteryGift *gift, FieldScriptEnv *env);
BOOL ScrcmdPokemonFld_ReceiveGift(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_GetGiftReceiveType(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u16 GetActorIDOfMysteryGiftDeliveryMan(Field *field, GameData *gameData);
u16 IsMysteryGiftDeliveryManActorAvailable(Field *field);
FieldActor *FindMysteryGiftDeliveryManActor(Field *field);
s32 FindMysteryGiftDeliveryManNPCID(GameData *gameData);
BOOL ScrcmdPokemonFld_IsLibertyPassGift(MysteryGift *gift, u8 kind);
u32 ScrcmdPokemonFld_IsLandorusTherianGift(FieldScriptEnv *env, MysteryGift *gift, u8 kind);
PartyPkm *ScrcmdPokemonFld_CreateGiftMon(FieldScriptEnv *env, MysteryGift *gift);
BOOL doesPartyHaveSpace(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void ScrcmdPokemonFld_ReceiveGiftMon(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_GetGiftMonReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_PrepareGiftMonMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 ScrcmdPokemonFld_PrepareGiftMonFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
BOOL ScrcmdPokemonFld_CanReceiveGiftItem(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void ScrcmdPokemonFld_ReceiveGiftItem(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_GetGiftItemReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_PrepareGiftItemMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 ScrcmdPokemonFld_PrepareGiftItemFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
BOOL ScrcmdPokemonFld_CanReceiveGiftFlag(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void ScrcmdPokemonFld_ReceiveGiftFlag(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_GetGiftFlagReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_PrepareGiftFlagMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 ScrcmdPokemonFld_PrepareGiftFlagFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u8 ScrcmdPokemonFld_GetGiftOneShotPower(MysteryGift *gift);
BOOL ScrcmdPokemonFld_CanReceiveGiftOneShot(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void ScrcmdPokemonFld_ReceiveGiftOneShot(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_GetGiftOneShotReceiveType(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 ScrcmdPokemonFld_PrepareGiftOneShotMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 ScrcmdPokemonFld_PrepareGiftOneShotFollowUpMessage(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
MysteryGift *PdwPostman_GetMysteryCard(MysteryGiftSave *save, u32 slot, MysteryGift *gift);
MysteryGift *PdwPostman_FindMysteryCard(MysteryGiftSave *save, u32 *slot, MysteryGift *gift);
void PdwPostman_SetCardDelivered(MysteryGiftSave *save, u32 slot);
u8 PdwPostman_GetMysteryCardKind(MysteryGiftSave *save);

#endif // POKEBW2_FIELD_PDW_POSTMAN_H
