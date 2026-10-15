#include "types.h"
#include "app/mystery/mystery_check.h"
#include "app/mystery/mystery_gift_data.h"
#include "gfl/heap.h"
#include "gfl/str.h"

// Mystery Gift's check of a received gift before it is offered: its texts are terminated, its ID and kind are in
// range and what it gives is valid. The name is ours (mystery_check.c); the ROM gives none for this file

static u32 MysteryCheck_CountGiftErrors(MysteryGift *gift, GameData *gameData, HeapID heapId);

u32 MysteryCheck_CountErrors(MysteryGiftRecvData *recv, GameData *gameData, HeapID heapId) {
    u32 errors = 0;
    BOOL unterminated = TRUE;
    int i;

    for (i = 0; i < 253; i++) {
        if (recv->text[i] == GFL_StrBufGetTerminator()) {
            unterminated = FALSE;
        }
    }
    if (unterminated) {
        errors++;
    }
    return errors + MysteryCheck_CountGiftErrors(&recv->gift, gameData, heapId);
}

static u32 MysteryCheck_CountGiftErrors(MysteryGift *gift, GameData *gameData, HeapID heapId) {
    u32 errors = 0;
    BOOL unterminated = TRUE;
    int i;
    PartyPkm *pkm;

    for (i = 0; i < 37; i++) {
        if (gift->title[i] == GFL_StrBufGetTerminator()) {
            unterminated = FALSE;
        }
    }
    if (unterminated) {
        errors++;
    }
    if (gift->id >= 2048) {
        errors++;
    }
    if (gift->kind == 0 || gift->kind >= 5) {
        errors++;
    }
    switch (gift->kind) {
    case 1:
        pkm = Mystery_CreateGiftPokemon(gift, heapId, gameData);
        if (pkm == NULL) {
            errors++;
        } else {
            GFL_HeapFree(pkm);
        }
        break;
    case 2:
        if (Mystery_GetGiftItem(gift) == MYSTERY_GIFT_ITEM_NONE) {
            errors++;
        }
        break;
    case 3:
        if (Mystery_GetGiftPassPower(gift) == MYSTERY_GIFT_PASS_POWER_NONE) {
            errors++;
        }
        break;
    case 4:
        if (Mystery_GetGiftOneShotPower(gift) == MYSTERY_GIFT_ONE_SHOT_NONE) {
            errors++;
        }
        break;
    }
    return errors;
}
