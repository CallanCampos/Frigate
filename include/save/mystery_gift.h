#ifndef POKEBW2_SAVE_MYSTERY_GIFT_H
#define POKEBW2_SAVE_MYSTERY_GIFT_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A mystery gift card, 0xcc bytes
struct MysteryGift {
    // The item, Pass Power or other thing the gift gives. A Pokémon gift keeps the Pokémon's data from here
    u32 value;
    u8 unk04[0x5c];
    u16 title[37];
    u8 unkAA[2];
    // The date the card was received, as year << 16 | month << 8 | day
    s32 date;
    u16 id;
    // Which of the gift messages the card shows
    u8 msgIndex;
    // What the gift gives, 1 for a Pokémon and 2 for an item
    u8 kind;
    // Received at most once per game, by its ID
    u8 once : 1;
    // Picked up from the delivery man; only such cards can be thrown away
    u8 delivered : 1;
    u8 unkB4_2 : 6;
    u8 unkB5[0x17];
};

// A gift as it is received, with its card's text
typedef struct {
    MysteryGift gift;
    // The games that may receive the gift, 1 << GAME_VERSION for each
    u32 versions;
    u16 text[253];
    // Shown with the special effect, with particles
    u8 special;
    u8 unk2CB;
    u16 unk2CC;
    // getCRC16 of everything before it
    u16 crc;
} MysteryGiftRecvData;

// The Pokémon of a Pokémon gift, which the card keeps from its start. 0xff, or 0, leaves a value to the game
typedef struct {
    u32 trainerId;
    u32 version;
    u32 personality;
    // A bit for each of the ribbons the Pokémon has
    u16 ribbons;
    u16 ball;
    u16 heldItem;
    u16 moves[4];
    u16 species;
    u8 form;
    u8 language;
    u16 nickname[11];
    u8 nature;
    u8 gender;
    // 0 or 1, the hidden ability (2), either at random (4), or 3
    u8 abilityType;
    // 0 for any, 1 shiny, 2 never shiny
    u8 shininess;
    u16 eggLocation;
    u16 metLocation;
    u8 metLevel;
    u8 contest[6];
    u8 ivs[6];
    u8 pad49;
    u16 otName[8];
    u8 otGender;
    u8 level;
    u8 isEgg;
    u8 unk5D[0x4f];
    // The date the gift was made, as year << 16 | month << 8 | day
    s32 date;
} MysteryGiftPokemon;

// The save's block of mystery gifts, which mgEncryptData encrypts
#define SAVE_BLOCK_MYSTERY_GIFT 0x22

#define MG_CARD_MAX 12
#define MG_CARD_MAX_OUTSIDE 3
#define MG_ID_MAX 0x800

typedef struct {
    // A bit for each gift ID received
    u8 receivedFlags[MG_ID_MAX / 8];
    MysteryGift cards[MG_CARD_MAX];
    // Kept by mgEncryptData at the end of the data it encrypts
    u16 checksum;
    u16 key;
} MysteryGiftSaveData; // size: 0xa94

// What mgEncryptData and mgDecryptData encrypt: the save's block, or the gifts kept outside the save (save_outside.c)
#define MG_DATA_OUTSIDE 0
#define MG_DATA_SAVE 1

// The mystery gifts the game works on: the save's block, or the gifts outside the save when there is no save yet
struct MysteryGiftSave {
    // A MysteryGiftSaveData, or a SaveOutsideGifts for MG_DATA_OUTSIDE
    void *data;
    u32 kind;
    // The card's data outside the save, for MG_DATA_OUTSIDE
    SaveOutside *outside;
}; // size: 0xc

void mgEncryptData(void *data, u32 kind);
void mgDecryptData(void *data, u32 kind);

// The size of the save's block and its init
u32 MysteryGiftSaveData_GetSize(void);
void MysteryGiftSaveData_Init(MysteryGiftSaveData *data);
// Whether a card's kind is one of the gifts
BOOL MysteryGift_IsValidKind(u32 kind);
// Loads the mystery gift save into a new buffer, and frees it. Function name from swan. Mode 1 takes the gifts
// outside the save when there is no save yet; any other mode takes the save's block
MysteryGiftSave *mysteryGiftBlock(SaveControl *save, u32 mode, HeapID heapId);
void MysteryGiftSave_Free(MysteryGiftSave *save);
// The same, in a MysteryGiftSave of the caller's
void MysteryGiftSave_Load(MysteryGiftSave *giftSave, SaveControl *save, u32 mode, HeapID heapId);
void MysteryGiftSave_Unload(MysteryGiftSave *save);
// Copies the card in the slot into gift
BOOL MysteryGiftSave_GetCard(MysteryGiftSave *save, u32 slot, MysteryGift *gift);
// Whether the slot has a card
BOOL MysteryGiftSave_IsSlotUsed(MysteryGiftSave *save, u32 slot);
// Whether the gift of the slot was picked up from the delivery man, which MysteryGiftSave_SetCardDelivered marks
BOOL MysteryGiftSave_IsCardDelivered(MysteryGiftSave *save, u32 slot);
void MysteryGiftSave_SetCardDelivered(MysteryGiftSave *save, u32 slot);
// Saves the gift in the first free slot, and returns FALSE when there is none
BOOL MysteryGiftSave_AddCard(MysteryGiftSave *save, const MysteryGift *gift);
// Throws away the card in the slot
void MysteryGiftSave_RemoveCard(MysteryGiftSave *save, u32 slot);
// Whether the album has a free slot
BOOL MysteryGiftSave_HasFreeSlot(MysteryGiftSave *save);
// Whether the album has any card
BOOL MysteryGiftSave_HasAnyCard(MysteryGiftSave *save);
// Swaps the cards of two slots
void MysteryGiftSave_SwapCards(MysteryGiftSave *save, u32 slot1, u32 slot2);
// Whether the gift with the ID was received, and the marking of it
BOOL MysteryGiftSave_IsIdReceived(MysteryGiftSave *save, u32 id);
void MysteryGiftSave_SetIdReceived(MysteryGiftSave *save, u32 id);
// Whether the received data's CRC is right
BOOL MysteryGiftRecvData_CheckCRC(const MysteryGiftRecvData *data);
// Starts and checks the save of the album
void MysteryGiftSave_StartSave(MysteryGiftSave *save, GameData *gameData);
u32 MysteryGiftSave_SaveStep(MysteryGiftSave *save, GameData *gameData);
// The number of card slots, and the MG_DATA_* kind
u32 MysteryGiftSave_GetCardMax(MysteryGiftSave *save);
u32 MysteryGiftSave_GetKind(MysteryGiftSave *save);

#endif // POKEBW2_SAVE_MYSTERY_GIFT_H
