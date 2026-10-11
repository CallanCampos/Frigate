#ifndef POKEBW2_APP_ZUKAN_NICKNAME_H
#define POKEBW2_APP_ZUKAN_NICKNAME_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The nickname question of the Pokédex registration, overlay 297 (zukan_nickname.c, by the ROM's string): "Give a
// nickname to the caught Pokémon?" with the yes or no of the battle's lower screen, and, if the answer is no, the
// message about the box that the Pokémon goes to

typedef struct ZukanNickname ZukanNickname;

// What the player answered
#define ZUKAN_NICKNAME_NO 0
#define ZUKAN_NICKNAME_YES 1
// Not yet
#define ZUKAN_NICKNAME_UNANSWERED 2

// boxMessage is the message about the box (see ZukanTorokuParam), or NULL for none
ZukanNickname *ZukanNickname_Create(HeapID heapId, PartyPkm *pkm, StrBuf *boxMessage, BoxSaveAccessor *boxes, u32 box,
                                    ClActUnit *unit, Font *font, PrintQueue *printQueue, GameData *gameData);
void ZukanNickname_Free(ZukanNickname *nickname);
// Returns whether the question has been answered and its messages are done
BOOL ZukanNickname_Update(ZukanNickname *nickname);
// Draws the window and starts the question
void ZukanNickname_Start(ZukanNickname *nickname);
u32 ZukanNickname_GetAnswer(ZukanNickname *nickname);

#endif // POKEBW2_APP_ZUKAN_NICKNAME_H
