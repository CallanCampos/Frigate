#ifndef POKEBW2_APP_POKEDEX_NICKNAME_H
#define POKEBW2_APP_POKEDEX_NICKNAME_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The nickname question of the Pokédex registration, overlay 297 (pokedex_nickname.c, zukan_nickname.c by the ROM's
// string): "Give a nickname to the caught Pokémon?" with the yes or no of the battle's lower screen, and, if the answer
// is no, the message about the box that the Pokémon goes to

typedef struct PokedexNickname PokedexNickname;

// What the player answered
enum {
    POKEDEX_NICKNAME_NO,
    POKEDEX_NICKNAME_YES,
    // Not yet
    POKEDEX_NICKNAME_UNANSWERED,
};

// boxMessage is the message about the box (see PokedexRegistrationParam), or NULL for none
PokedexNickname *PokedexNickname_Create(HeapID heapId, PartyPkm *pkm, StrBuf *boxMessage, BoxSaveAccessor *boxes,
                                        u32 box, ClActUnit *unit, Font *font, PrintQueue *printQueue,
                                        GameData *gameData);
void PokedexNickname_Free(PokedexNickname *nickname);
// Returns whether the question has been answered and its messages are done
BOOL PokedexNickname_Update(PokedexNickname *nickname);
// Draws the window and starts the question
void PokedexNickname_Start(PokedexNickname *nickname);
// A POKEDEX_NICKNAME_*
u32 PokedexNickname_GetAnswer(PokedexNickname *nickname);

#endif // POKEBW2_APP_POKEDEX_NICKNAME_H
