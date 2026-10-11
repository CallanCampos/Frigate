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
ZukanInfo *func_ov296_0219d6e0(HeapID heapId, PartyPkm *pkm, BOOL national, BOOL caught, u32 mode, u32 engine,
                               u8 bgPriority, ClActUnit *unit, Font *font, PrintQueue *printQueue, MsgData *msgData0,
                               MsgData *msgData1, MsgData *msgData2);
// sex, rare and form are what the Pokédex shows of the species (func_0200d3c8), personality a Spinda's spots. The
// print queue is not used, the entry makes its own. The message files are the species categories, the heights and the
// weights that the caller has loaded already, or NULL to load them
ZukanInfo *func_ov296_0219d768(HeapID heapId, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL national,
                               BOOL caught, u32 mode, u32 engine, u8 bgPriority, ClActUnit *unit, Font *font,
                               PrintQueue *printQueue, MsgData *msgData0, MsgData *msgData1, MsgData *msgData2);
void func_ov296_0219dbcc(ZukanInfo *info);
void func_ov296_0219dc74(ZukanInfo *info);
// Frees the windows and what the entry shows, before func_ov296_0219dbcc frees the rest
void func_ov296_0219ddcc(ZukanInfo *info);
// Mode 0 waits for the caller to say that it is ready before the sprite slides in, and mode 1 for it to ask for the
// Pokémon's cry
void func_ov296_0219de14(ZukanInfo *info);
void func_ov296_0219de1c(ZukanInfo *info);
// Whether the sprite is sliding in, and whether the entry has finished its animation
BOOL func_ov296_0219de24(ZukanInfo *info);
BOOL func_ov296_0219de34(ZukanInfo *info);
// Starts mode 0's animation
void func_ov296_0219de44(ZukanInfo *info);
// Shows another Pokémon
void func_ov296_0219de50(ZukanInfo *info, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL caught);
// Shows the entry in another language
void func_ov296_0219df4c(ZukanInfo *info, u32 language);
void func_ov296_0219dfb0(ZukanInfo *info, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL caught,
                         u32 language);
void func_ov296_0219e0c8(ZukanInfo *info);
void func_ov296_0219e114(ZukanInfo *info);

#endif // POKEBW2_APP_ZUKAN_INFO_H
