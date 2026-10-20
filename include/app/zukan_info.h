#ifndef POKEBW2_APP_ZUKAN_INFO_H
#define POKEBW2_APP_ZUKAN_INFO_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "struct_decls.h"
#include "system/printsys.h"

// A Pokémon's Pokédex entry, overlay 296 (zukan_info.c): its sprite, name, type, height, weight and description, in
// the game's language or another one. The Pokédex's detail screen draws one on each screen. None of these functions
// has a name yet

#define OVERLAY_ZUKAN_INFO OVERLAY_ID(296)

typedef struct ZukanInfo ZukanInfo;

// The same for a Pokémon of the party. national is whether the national Pokédex numbers it, caught whether its entry
// is shown or only its sprite, mode 0, 1 or 2 how it is shown (2 by the detail screen), engine 0 for the main screen
// and 1 for the sub screen
ZukanInfo *ZukanInfo_CreateForPartyPkm(HeapID heapId, PartyPkm *pkm, BOOL national, BOOL caught, u32 mode, u32 engine,
                                       u8 bgPriority, ClActUnit *unit, Font *font, PrintQueue *printQueue,
                                       MsgData *msgData0, MsgData *msgData1, MsgData *msgData2);
// sex, rare and form are what the Pokédex shows of the species (PokeDex_GetShownSexRareForm), personality a Spinda's
// spots. The print queue is not used, the entry makes its own. The message files are the species categories, the
// heights and the weights that the caller has loaded already, or NULL to load them
ZukanInfo *ZukanInfo_Create(HeapID heapId, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL national,
                            BOOL caught, u32 mode, u32 engine, u8 bgPriority, ClActUnit *unit, Font *font,
                            PrintQueue *printQueue, MsgData *msgData0, MsgData *msgData1, MsgData *msgData2);
void ZukanInfo_Free(ZukanInfo *info);
void ZukanInfo_Update(ZukanInfo *info);
// Frees the windows and what the entry shows, before ZukanInfo_Free frees the rest
void ZukanInfo_FreeEntry(ZukanInfo *info);
// Mode 0 waits for the caller to say that it is ready before the sprite slides in, and mode 1 for it to ask for the
// Pokémon's cry
void ZukanInfo_SetReady(ZukanInfo *info);
void ZukanInfo_RequestCry(ZukanInfo *info);
// Whether the sprite is sliding in, and whether the entry has finished its animation
BOOL ZukanInfo_IsSlidingIn(ZukanInfo *info);
BOOL ZukanInfo_IsDone(ZukanInfo *info);
// Starts mode 0's animation
void ZukanInfo_Start(ZukanInfo *info);
// Shows another Pokémon
void ZukanInfo_SetPokemon(ZukanInfo *info, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL caught);
// Shows the entry in another language
void ZukanInfo_SetLanguage(ZukanInfo *info, u32 language);
void ZukanInfo_SetPokemonInLanguage(ZukanInfo *info, u16 species, u16 form, u16 sex, u16 rare, u32 personality,
                                    BOOL caught, u32 language);
void ZukanInfo_Hide(ZukanInfo *info);
void ZukanInfo_Show(ZukanInfo *info);

#endif // POKEBW2_APP_ZUKAN_INFO_H
