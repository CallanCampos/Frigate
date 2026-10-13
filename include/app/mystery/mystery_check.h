#ifndef POKEBW2_APP_MYSTERY_MYSTERY_CHECK_H
#define POKEBW2_APP_MYSTERY_MYSTERY_CHECK_H

// Mystery Gift's check of a received gift (ov197, mystery_check.c, a descriptive name). Our names; swan has none for
// this overlay

#include "types.h"
#include "gfl/heap.h"
#include "save/mystery_gift.h"
#include "struct_decls.h"

// The number of things wrong with the gift: unterminated texts, an ID or kind out of range, or a Pokémon that can't be
// made
u32 MysteryCheck_CountErrors(MysteryGiftRecvData *recv, GameData *gameData, HeapID heapId);

#endif // POKEBW2_APP_MYSTERY_MYSTERY_CHECK_H
