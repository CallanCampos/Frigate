#ifndef POKEBW2_FIELD_PDW_POSTMAN_H
#define POKEBW2_FIELD_PDW_POSTMAN_H

// The postman of overlay 33's pdw_postman.c, who hands out the items sent from the Pokémon Dream World and the
// mystery gifts. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "save/mystery_gift.h"
#include "system/game_event.h"
#include "struct_decls.h"

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
    void *window;
    u32 unk10;
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
    u32 (*unk0C)(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
    // These set the words of a message and return the message
    u32 (*unk10)(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
    u32 (*unk14)(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
} PdwPostmanGiftHandler;

PdwPostmanItemWindow *func_ov033_02177998(Field *field, PdwPostmanItem *items, u32 count);
void func_ov033_02177a28(PdwPostmanItemWindow *work);
void func_ov033_02177a60(PdwPostmanItemWindow *work);
GameEventReturnCode func_ov033_02177b08(GameEvent *event, u32 *state, void *data);
PdwPostmanItem *func_ov033_02177bd4(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld, u32 *count);
void func_ov033_02177c48(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld);
u32 func_ov033_02177c8c(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld);
u16 func_ov033_02177cd4(GameData *gameData, HeapID heapId, DreamWorldSave *dreamWorld, u32 position);
GameEvent *func_ov033_02177d28(GameSystem *gsys);
BOOL func_ov033_02177d78(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_02177ed0(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02177ef4(u32 kind, MysteryGift *gift, FieldScriptEnv *env);
u32 func_ov033_02177f28(u32 kind, MysteryGift *gift, FieldScriptEnv *env);
BOOL func_ov033_02177f5c(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02177f84(u32 kind, FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u16 GetActorIDOfMysteryGiftDeliveryMan(Field *field, GameData *gameData);
u16 IsMysteryGiftDeliveryManActorAvailable(Field *field);
FieldActor *FindMysteryGiftDeliveryManActor(Field *field);
s32 FindMysteryGiftDeliveryManNPCID(GameData *gameData);
BOOL func_ov033_02178074(MysteryGift *gift, u8 kind);
u32 func_ov033_021780a4(FieldScriptEnv *env, MysteryGift *gift, u8 kind);
PartyPkm *func_ov033_021780d8(FieldScriptEnv *env, MysteryGift *gift);
BOOL doesPartyHaveSpace(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void func_ov033_02178110(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178154(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178180(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 func_ov033_021781e0(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
BOOL func_ov033_021781e4(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void func_ov033_021781e8(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178218(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178230(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 func_ov033_02178260(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
BOOL func_ov033_02178274(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void func_ov033_02178278(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178290(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178294(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 func_ov033_021782cc(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u8 func_ov033_021782d0(MysteryGift *gift);
BOOL func_ov033_021782f0(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
void func_ov033_021782f4(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178334(FieldScriptEnv *env, GameData *gameData, MysteryGift *gift);
u32 func_ov033_02178338(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
u32 func_ov033_02178374(WordSet *wordSet, MysteryGift *gift, FieldScriptEnv *env);
MysteryGift *func_ov033_021783a8(MysteryGiftSave *save, u32 slot, MysteryGift *gift);
MysteryGift *func_ov033_021783f8(MysteryGiftSave *save, u32 *slot, MysteryGift *gift);
void func_ov033_02178420(MysteryGiftSave *save, u32 slot);
u8 func_ov033_02178428(MysteryGiftSave *save);

#endif // POKEBW2_FIELD_PDW_POSTMAN_H
